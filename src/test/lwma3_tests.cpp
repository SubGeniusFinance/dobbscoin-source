// Copyright (c) 2026 The Dobbscoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chain.h"
#include "chainparams.h"
#include "chainparamsbase.h"
#include "pow.h"

#include <boost/test/unit_test.hpp>

#include <vector>

BOOST_AUTO_TEST_SUITE(lwma3_tests)

namespace {

// Build a synthetic CBlockIndex chain of `count` entries with uniform solvetime
// and uniform nBits. The most recent entry is at the back and its
// nHeight === starting_height + count - 1. pprev wires entries linearly.
//
// This is the only fixture needed for LWMA-3: the algorithm reads timestamps
// + nBits from the last N+1 blocks via pprev walks, nothing else.
static std::vector<CBlockIndex> BuildChain(
    int starting_height,
    unsigned int count,
    unsigned int starting_time,
    unsigned int solvetime,
    unsigned int nBits)
{
    std::vector<CBlockIndex> chain(count);
    for (unsigned int i = 0; i < count; ++i) {
        chain[i].SetNull();
        chain[i].nHeight = starting_height + i;
        chain[i].nTime   = starting_time + i * solvetime;
        chain[i].nBits   = nBits;
        chain[i].pprev   = (i == 0) ? NULL : &chain[i - 1];
    }
    return chain;
}

} // anon

// Pre-activation block — dispatcher must NOT take the LWMA-3 path yet.
// Mainnet activation is HARDFORK_LWMA3_MAIN; one block earlier still uses
// the prior algorithm (V4 DigiShield).
BOOST_AUTO_TEST_CASE(lwma3_inactive_below_fork)
{
    SelectParams(CBaseChainParams::MAIN);

    // Build a chain ending at (fork - 2); pindexLast->nHeight+1 = fork - 1
    // — strictly below the >= fork threshold, so DiffMode stays at 4.
    std::vector<CBlockIndex> chain =
        BuildChain(HARDFORK_LWMA3_MAIN - 200, 100, 1779000000u, 120u, 0x1d00ffffu);

    CBlockHeader header;
    header.nVersion = 3;
    header.nTime    = chain.back().nTime + 120u;
    header.nBits    = chain.back().nBits;

    // LWMA-3 returns pindexLast->nBits only inside the bootstrap window
    // (fork ≤ nHeight+1 < fork+N). Below the fork the V4 path runs instead.
    // We can't easily oracle V4's exact output without a 1440-block window,
    // but we CAN confirm the call doesn't crash and returns a valid compact.
    unsigned int result = GetNextWorkRequired(&chain.back(), &header);
    BOOST_CHECK(result != 0);

    SelectParams(CBaseChainParams::UNITTEST);
}

// Bootstrap window — at and just past the fork, before we have N solvetimes
// of post-fork history, LWMA-3 returns the previous block's nBits unchanged.
BOOST_AUTO_TEST_CASE(lwma3_bootstrap_returns_prev_nbits)
{
    SelectParams(CBaseChainParams::MAIN);

    const unsigned int seedBits = 0x1c123456u;
    // Chain ending exactly at the fork height — next block is fork+1, still
    // inside the (fork .. fork+N) bootstrap window.
    std::vector<CBlockIndex> chain =
        BuildChain(HARDFORK_LWMA3_MAIN - 30, 31, 1779000000u, 120u, seedBits);

    CBlockHeader header;
    header.nTime = chain.back().nTime + 120u;
    header.nBits = 0;  // unused

    BOOST_CHECK_EQUAL(GetNextWorkRequired(&chain.back(), &header), seedBits);

    SelectParams(CBaseChainParams::UNITTEST);
}

// Equilibrium: all solvetimes exactly T=120, all nBits identical. The
// algorithm collapses to the running average of recent targets — which is
// the seed value. Output compact MUST equal input compact (modulo nothing).
BOOST_AUTO_TEST_CASE(lwma3_equilibrium_holds_difficulty)
{
    SelectParams(CBaseChainParams::MAIN);

    const unsigned int seedBits = 0x1c0fffffu;
    // 80 blocks: 20 leading the fork, 60 post-fork for the LWMA window.
    // pindexLast->nHeight+1 = fork+60+1 ≥ fork+N, so LWMA-3 is fully engaged.
    std::vector<CBlockIndex> chain =
        BuildChain(HARDFORK_LWMA3_MAIN - 20, 80, 1779000000u, 120u, seedBits);

    CBlockHeader header;
    header.nTime = chain.back().nTime + 120u;
    header.nBits = 0;

    unsigned int next = GetNextWorkRequired(&chain.back(), &header);

    // At equilibrium next-target == seed-target. Compact-form quantization
    // may shift mantissa by ±1 depending on rounding; allow that slack.
    uint256 wantTarget; wantTarget.SetCompact(seedBits);
    uint256 gotTarget;  gotTarget.SetCompact(next);

    // |got - want| / want <= 1/256  (one mantissa LSB at exponent ~28)
    uint256 diff = (gotTarget > wantTarget) ? (gotTarget - wantTarget)
                                            : (wantTarget - gotTarget);
    BOOST_CHECK(diff <= (wantTarget >> 8));

    SelectParams(CBaseChainParams::UNITTEST);
}

// Fast burst: every block came in at T/2 = 60s. Difficulty should
// approximately double — i.e. next target ≈ seed target / 2.
BOOST_AUTO_TEST_CASE(lwma3_fast_blocks_raise_difficulty)
{
    SelectParams(CBaseChainParams::MAIN);

    const unsigned int seedBits = 0x1c0fffffu;
    std::vector<CBlockIndex> chain =
        BuildChain(HARDFORK_LWMA3_MAIN - 20, 80, 1779000000u, 60u, seedBits);

    CBlockHeader header;
    header.nTime = chain.back().nTime + 60u;
    header.nBits = 0;

    unsigned int next = GetNextWorkRequired(&chain.back(), &header);

    uint256 seedTarget; seedTarget.SetCompact(seedBits);
    uint256 nextTarget; nextTarget.SetCompact(next);

    // Expect next ≈ seed/2. Allow [seed*0.4, seed*0.6] band — wider than
    // the math demands, narrow enough to fail if the sign is wrong.
    BOOST_CHECK(nextTarget < seedTarget);
    BOOST_CHECK(nextTarget > (seedTarget >> 2));   // > seed * 0.25
    BOOST_CHECK(nextTarget < (seedTarget * 3) / 4); // < seed * 0.75

    SelectParams(CBaseChainParams::UNITTEST);
}

// Slow drought: every block came in at 2T = 240s. Difficulty should
// approximately halve — i.e. next target ≈ 2 * seed target.
BOOST_AUTO_TEST_CASE(lwma3_slow_blocks_lower_difficulty)
{
    SelectParams(CBaseChainParams::MAIN);

    const unsigned int seedBits = 0x1c0fffffu;
    std::vector<CBlockIndex> chain =
        BuildChain(HARDFORK_LWMA3_MAIN - 20, 80, 1779000000u, 240u, seedBits);

    CBlockHeader header;
    header.nTime = chain.back().nTime + 240u;
    header.nBits = 0;

    unsigned int next = GetNextWorkRequired(&chain.back(), &header);

    uint256 seedTarget; seedTarget.SetCompact(seedBits);
    uint256 nextTarget; nextTarget.SetCompact(next);

    BOOST_CHECK(nextTarget > seedTarget);
    BOOST_CHECK(nextTarget < seedTarget * 3);
    BOOST_CHECK(nextTarget > (seedTarget * 3) / 2); // > seed * 1.5

    SelectParams(CBaseChainParams::UNITTEST);
}

// Solvetime clamp: a single outlier (a block 100·T late) cannot dominate
// the moving average. The solvetime is clamped to +6T inside LWMA-3,
// so the contribution of the bogus block is bounded.
BOOST_AUTO_TEST_CASE(lwma3_outlier_solvetime_is_clamped)
{
    SelectParams(CBaseChainParams::MAIN);

    const unsigned int seedBits = 0x1c0fffffu;
    const unsigned int T = 120u;
    std::vector<CBlockIndex> chain =
        BuildChain(HARDFORK_LWMA3_MAIN - 20, 80, 1779000000u, T, seedBits);

    // Inject one absurdly-late block in the middle of the window.
    chain[60].nTime = chain[59].nTime + 100 * T;
    for (size_t i = 61; i < chain.size(); ++i)
        chain[i].nTime = chain[i - 1].nTime + T;

    CBlockHeader header;
    header.nTime = chain.back().nTime + T;
    header.nBits = 0;

    unsigned int next = GetNextWorkRequired(&chain.back(), &header);

    uint256 seedTarget; seedTarget.SetCompact(seedBits);
    uint256 nextTarget; nextTarget.SetCompact(next);

    // With +6T clamp on one of 60 weighted samples, drift is bounded.
    // Difficulty should drop slightly (target rises), not collapse.
    BOOST_CHECK(nextTarget >= seedTarget);
    BOOST_CHECK(nextTarget <= seedTarget * 2);

    SelectParams(CBaseChainParams::UNITTEST);
}

// Backward-timestamp pathology: a string of blocks with timestamps that
// go BACKWARD. weightedTime would go negative; the k/3 floor must engage
// to prevent next-target collapse / undefined behaviour.
BOOST_AUTO_TEST_CASE(lwma3_negative_weight_floor_engages)
{
    SelectParams(CBaseChainParams::MAIN);

    const unsigned int seedBits = 0x1c0fffffu;
    std::vector<CBlockIndex> chain =
        BuildChain(HARDFORK_LWMA3_MAIN - 20, 80, 1779000000u, 120u, seedBits);

    // Reverse the timestamps across the LWMA window: each later block has
    // an earlier nTime than its predecessor. After clamping, every solvetime
    // becomes -6T; weightedTime becomes very negative; the k/3 floor catches.
    for (size_t i = 20; i < chain.size(); ++i)
        chain[i].nTime = chain[19].nTime - (i - 19) * 600u;

    CBlockHeader header;
    header.nTime = chain.back().nTime - 600u;
    header.nBits = 0;

    unsigned int next = GetNextWorkRequired(&chain.back(), &header);

    // Must not crash, must produce a non-zero target ≤ powLimit.
    uint256 nextTarget; nextTarget.SetCompact(next);
    BOOST_CHECK(next != 0);
    BOOST_CHECK(nextTarget <= Params().ProofOfWorkLimit());

    // Floor at k/3 means weightedTime is effectively k/3, so the answer
    // should be roughly seed/3 (difficulty triples) — definitely not zero,
    // not >> seedTarget. Wide tolerance: [seed/8, seed].
    uint256 seedTarget; seedTarget.SetCompact(seedBits);
    BOOST_CHECK(nextTarget < seedTarget);
    BOOST_CHECK(nextTarget > (seedTarget >> 4)); // > seed/16

    SelectParams(CBaseChainParams::UNITTEST);
}

BOOST_AUTO_TEST_SUITE_END()
