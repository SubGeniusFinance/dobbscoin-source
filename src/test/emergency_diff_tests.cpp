// Copyright (c) 2026 The Dobbscoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chain.h"
#include "chainparams.h"
#include "chainparamsbase.h"
#include "pow.h"
#include "primitives/block.h"
#include "uint256.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(emergency_diff_tests)

namespace {

// Build the minimum fixture IsEmergencyDifficultyBlock reads: a CBlockIndex
// with nHeight (gates activation) and nTime (gates the >6h gap check).
static CBlockIndex MakePrev(int height, uint32_t nTime)
{
    CBlockIndex idx;
    idx.SetNull();
    idx.nHeight = height;
    idx.nTime   = nTime;
    return idx;
}

static unsigned int PowLimitCompact()
{
    return Params().ProofOfWorkLimit().GetCompact();
}

} // anon

// Pre-activation: a block claiming min-difficulty after a 24h stall is NOT
// granted the emergency exemption. The rule is height-gated; v0.12.0 and
// earlier nodes must reject such a block as bad-diffbits.
BOOST_AUTO_TEST_CASE(emergency_diff_inactive_below_fork)
{
    SelectParams(CBaseChainParams::MAIN);

    // pindexPrev->nHeight + 1 == HARDFORK_EMERGENCY_DIFF_MAIN - 1, strictly
    // below the >= fork threshold.
    CBlockIndex prev = MakePrev(HARDFORK_EMERGENCY_DIFF_MAIN - 2, 1800000000u);

    CBlockHeader header;
    header.nTime = prev.nTime + 24 * 60 * 60;  // 24h gap — well past the 6h floor
    header.nBits = PowLimitCompact();

    BOOST_CHECK(!IsEmergencyDifficultyBlock(header, &prev));

    SelectParams(CBaseChainParams::UNITTEST);
}

// Post-fork, normal-cadence block at min-difficulty — REJECTED. A short
// solvetime cannot be paired with min-difficulty bits just because the
// activation height has been crossed. The 6h gap is load-bearing.
BOOST_AUTO_TEST_CASE(emergency_diff_normal_block_at_min_diff_rejected)
{
    SelectParams(CBaseChainParams::MAIN);

    CBlockIndex prev = MakePrev(HARDFORK_EMERGENCY_DIFF_MAIN - 1, 1800000000u);

    CBlockHeader header;
    header.nTime = prev.nTime + 120u;          // T = 2 min, healthy chain
    header.nBits = PowLimitCompact();

    BOOST_CHECK(!IsEmergencyDifficultyBlock(header, &prev));

    SelectParams(CBaseChainParams::UNITTEST);
}

// Post-fork, 6h+ gap with min-difficulty bits — ACCEPTED. This is the
// canonical recovery path: hashrate departed, chain stalled, the first
// miner to publish anything at all at powLimit gets it accepted.
BOOST_AUTO_TEST_CASE(emergency_diff_long_gap_at_min_diff_accepted)
{
    SelectParams(CBaseChainParams::MAIN);

    CBlockIndex prev = MakePrev(HARDFORK_EMERGENCY_DIFF_MAIN - 1, 1800000000u);

    CBlockHeader header;
    header.nTime = prev.nTime + EMERGENCY_DIFFICULTY_GAP + 1;  // 6h + 1s
    header.nBits = PowLimitCompact();

    BOOST_CHECK(IsEmergencyDifficultyBlock(header, &prev));

    SelectParams(CBaseChainParams::UNITTEST);
}

// Post-fork, 6h+ gap but the miner found work at BETTER than min-difficulty —
// the emergency exemption does NOT apply. The block must validate under the
// normal nBits == GetNextWorkRequired() path. (The exemption is only a
// "safety valve"; a miner who can do real work after the stall is held to
// the real target.)
BOOST_AUTO_TEST_CASE(emergency_diff_long_gap_better_than_min_not_emergency)
{
    SelectParams(CBaseChainParams::MAIN);

    CBlockIndex prev = MakePrev(HARDFORK_EMERGENCY_DIFF_MAIN - 1, 1800000000u);

    // Build a target one mantissa-bit tighter than powLimit — definitely
    // != powLimit.GetCompact(), but still trivially mineable in the test
    // harness sense (we don't actually hash here).
    uint256 powLimit = Params().ProofOfWorkLimit();
    uint256 tighter  = powLimit >> 1;            // 2× harder than powLimit
    unsigned int tighterBits = tighter.GetCompact();
    BOOST_REQUIRE(tighterBits != PowLimitCompact());

    CBlockHeader header;
    header.nTime = prev.nTime + EMERGENCY_DIFFICULTY_GAP + 100;
    header.nBits = tighterBits;

    BOOST_CHECK(!IsEmergencyDifficultyBlock(header, &prev));

    SelectParams(CBaseChainParams::UNITTEST);
}

// Edge: exactly 6h. The predicate is strict-greater (`gap > 21600`), so
// 21600s on the nose is not enough. One second short of qualifying.
BOOST_AUTO_TEST_CASE(emergency_diff_exact_6h_gap_rejected)
{
    SelectParams(CBaseChainParams::MAIN);

    CBlockIndex prev = MakePrev(HARDFORK_EMERGENCY_DIFF_MAIN - 1, 1800000000u);

    CBlockHeader header;
    header.nTime = prev.nTime + EMERGENCY_DIFFICULTY_GAP;  // exactly 6h
    header.nBits = PowLimitCompact();

    BOOST_CHECK(!IsEmergencyDifficultyBlock(header, &prev));

    SelectParams(CBaseChainParams::UNITTEST);
}

// Defensive: pindexPrev == NULL should never crash and must return false.
// In live code this can't happen (ContextualCheckBlockHeader asserts prev)
// but the helper is exposed publicly and the guard is cheap.
BOOST_AUTO_TEST_CASE(emergency_diff_null_prev_safe)
{
    SelectParams(CBaseChainParams::MAIN);

    CBlockHeader header;
    header.nTime = 1800000000u;
    header.nBits = PowLimitCompact();

    BOOST_CHECK(!IsEmergencyDifficultyBlock(header, NULL));

    SelectParams(CBaseChainParams::UNITTEST);
}

BOOST_AUTO_TEST_SUITE_END()
