// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <math.h>
#include "pow.h"

#include "chain.h"
#include "chainparams.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

#include <memory>
#include <stdexcept>
#include <vector>

#include <openssl/bn.h>

typedef int64_t int64;
typedef uint64_t uint64;

namespace {

class bignum_error : public std::runtime_error
{
public:
    explicit bignum_error(const std::string& str) : std::runtime_error(str) {}
};

class CAutoBN_CTX
{
public:
    CAutoBN_CTX()
    {
        pctx = BN_CTX_new();
        if (pctx == NULL)
            throw bignum_error("CAutoBN_CTX : BN_CTX_new() returned NULL");
    }

    ~CAutoBN_CTX()
    {
        if (pctx != NULL)
            BN_CTX_free(pctx);
    }

    operator BN_CTX*() { return pctx; }

private:
    BN_CTX* pctx;
};

typedef std::unique_ptr<BIGNUM, decltype(&BN_clear_free)> BignumPtr;

static BignumPtr NewBignum()
{
    BIGNUM* bn = BN_new();
    if (bn == NULL)
        throw bignum_error("NewBignum : BN_new() returned NULL");
    BN_zero(bn);
    return BignumPtr(bn, BN_clear_free);
}

static BignumPtr CopyBignum(const BIGNUM* source)
{
    BignumPtr copy = NewBignum();
    if (!BN_copy(copy.get(), source))
        throw bignum_error("CopyBignum : BN_copy failed");
    return copy;
}

static BignumPtr BignumFromUint64(uint64_t n)
{
    unsigned char pch[sizeof(n) + 6];
    unsigned char* p = pch + 4;
    bool fLeadingZeroes = true;
    for (int i = 0; i < 8; i++)
    {
        unsigned char c = (n >> 56) & 0xff;
        n <<= 8;
        if (fLeadingZeroes)
        {
            if (c == 0)
                continue;
            if (c & 0x80)
                *p++ = 0;
            fLeadingZeroes = false;
        }
        *p++ = c;
    }
    unsigned int nSize = p - (pch + 4);
    pch[0] = (nSize >> 24) & 0xff;
    pch[1] = (nSize >> 16) & 0xff;
    pch[2] = (nSize >> 8) & 0xff;
    pch[3] = (nSize) & 0xff;

    BignumPtr bn = NewBignum();
    if (BN_mpi2bn(pch, p - pch, bn.get()) == NULL)
        throw bignum_error("BignumFromUint64 : BN_mpi2bn failed");
    return bn;
}

static BignumPtr BignumFromUint256(const uint256& n)
{
    unsigned char pch[sizeof(n) + 6];
    unsigned char* p = pch + 4;
    bool fLeadingZeroes = true;
    const unsigned char* pbegin = reinterpret_cast<const unsigned char*>(&n);
    const unsigned char* psrc = pbegin + sizeof(n);
    while (psrc != pbegin)
    {
        unsigned char c = *(--psrc);
        if (fLeadingZeroes)
        {
            if (c == 0)
                continue;
            if (c & 0x80)
                *p++ = 0;
            fLeadingZeroes = false;
        }
        *p++ = c;
    }
    unsigned int nSize = p - (pch + 4);
    pch[0] = (nSize >> 24) & 0xff;
    pch[1] = (nSize >> 16) & 0xff;
    pch[2] = (nSize >> 8) & 0xff;
    pch[3] = (nSize >> 0) & 0xff;

    BignumPtr bn = NewBignum();
    if (BN_mpi2bn(pch, p - pch, bn.get()) == NULL)
        throw bignum_error("BignumFromUint256 : BN_mpi2bn failed");
    return bn;
}

static uint256 Uint256FromBignum(const BIGNUM* bn)
{
    unsigned int nSize = BN_bn2mpi(bn, NULL);
    if (nSize < 4)
        return 0;

    std::vector<unsigned char> vch(nSize);
    BN_bn2mpi(bn, &vch[0]);
    if (vch.size() > 4)
        vch[4] &= 0x7f;

    uint256 n = 0;
    for (unsigned int i = 0, j = vch.size() - 1; i < sizeof(n) && j >= 4; i++, j--)
        reinterpret_cast<unsigned char*>(&n)[i] = vch[j];
    return n;
}

static BignumPtr BignumFromCompact(unsigned int nCompact)
{
    BignumPtr bn = NewBignum();
    unsigned int nSize = nCompact >> 24;
    bool fNegative = (nCompact & 0x00800000) != 0;
    unsigned int nWord = nCompact & 0x007fffff;
    if (nSize <= 3)
    {
        nWord >>= 8 * (3 - nSize);
        if (!BN_set_word(bn.get(), nWord))
            throw bignum_error("BignumFromCompact : BN_set_word failed");
    }
    else
    {
        if (!BN_set_word(bn.get(), nWord))
            throw bignum_error("BignumFromCompact : BN_set_word failed");
        if (!BN_lshift(bn.get(), bn.get(), 8 * (nSize - 3)))
            throw bignum_error("BignumFromCompact : BN_lshift failed");
    }
    BN_set_negative(bn.get(), fNegative);
    return bn;
}

static unsigned int CompactFromBignum(const BIGNUM* bn)
{
    unsigned int nSize = BN_num_bytes(bn);
    unsigned int nCompact = 0;
    if (nSize <= 3)
    {
        nCompact = BN_get_word(bn) << 8 * (3 - nSize);
    }
    else
    {
        BignumPtr shifted = NewBignum();
        if (!BN_rshift(shifted.get(), bn, 8 * (nSize - 3)))
            throw bignum_error("CompactFromBignum : BN_rshift failed");
        nCompact = BN_get_word(shifted.get());
    }
    if (nCompact & 0x00800000)
    {
        nCompact >>= 8;
        nSize++;
    }
    nCompact |= nSize << 24;
    nCompact |= (BN_is_negative(bn) ? 0x00800000 : 0);
    return nCompact;
}

static BignumPtr AddBignums(const BIGNUM* left, const BIGNUM* right)
{
    BignumPtr result = NewBignum();
    if (!BN_add(result.get(), left, right))
        throw bignum_error("AddBignums : BN_add failed");
    return result;
}

static BignumPtr SubtractBignums(const BIGNUM* left, const BIGNUM* right)
{
    BignumPtr result = NewBignum();
    if (!BN_sub(result.get(), left, right))
        throw bignum_error("SubtractBignums : BN_sub failed");
    return result;
}

static BignumPtr MultiplyBignums(const BIGNUM* left, const BIGNUM* right)
{
    CAutoBN_CTX ctx;
    BignumPtr result = NewBignum();
    if (!BN_mul(result.get(), left, right, ctx))
        throw bignum_error("MultiplyBignums : BN_mul failed");
    return result;
}

static BignumPtr DivideBignums(const BIGNUM* dividend, const BIGNUM* divisor)
{
    CAutoBN_CTX ctx;
    BignumPtr result = NewBignum();
    if (!BN_div(result.get(), NULL, dividend, divisor, ctx))
        throw bignum_error("DivideBignums : BN_div failed");
    return result;
}

unsigned int static KimotoGravityWell(const CBlockIndex* pindexLast, const CBlockHeader *pblock, uint64 TargetBlocksSpacingSeconds, uint64 PastBlocksMin, uint64 PastBlocksMax) {
    /* current difficulty formula, megacoin - kimoto gravity well */
    const CBlockIndex  *BlockLastSolved             = pindexLast;
    const CBlockIndex  *BlockReading                = pindexLast;
    const CBlockHeader *BlockCreating               = pblock;
                        BlockCreating               = BlockCreating;
    uint64              PastBlocksMass              = 0;
    int64               PastRateActualSeconds       = 0;
    int64               PastRateTargetSeconds       = 0;
    int64               LatestBlockTime             = BlockLastSolved->GetBlockTime();
    double              PastRateAdjustmentRatio     = double(1);
    BignumPtr           PastDifficultyAverage       = NewBignum();
    BignumPtr           PastDifficultyAveragePrev   = NewBignum();
    double              EventHorizonDeviation;
    double              EventHorizonDeviationFast;
    double              EventHorizonDeviationSlow;
    BignumPtr           bnProofOfWorkLimit          = BignumFromUint256(Params().ProofOfWorkLimit());

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || (uint64)BlockLastSolved->nHeight < PastBlocksMin) { return CompactFromBignum(bnProofOfWorkLimit.get()); }

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) { break; }
        PastBlocksMass++;

        BignumPtr currentDifficulty = BignumFromCompact(BlockReading->nBits);
        if (i == 1) {
            PastDifficultyAverage = CopyBignum(currentDifficulty.get());
        } else {
            BignumPtr delta = SubtractBignums(currentDifficulty.get(), PastDifficultyAveragePrev.get());
            BignumPtr divisor = BignumFromUint64(i);
            BignumPtr averageStep = DivideBignums(delta.get(), divisor.get());
            PastDifficultyAverage = AddBignums(averageStep.get(), PastDifficultyAveragePrev.get());
        }
        PastDifficultyAveragePrev = CopyBignum(PastDifficultyAverage.get());

        if (LatestBlockTime < BlockReading->GetBlockTime()) { LatestBlockTime = BlockReading->GetBlockTime(); }
        PastRateActualSeconds           = LatestBlockTime - BlockReading->GetBlockTime();
        PastRateTargetSeconds           = TargetBlocksSpacingSeconds * PastBlocksMass;
        PastRateAdjustmentRatio         = double(1);
        if (PastRateActualSeconds < 1) { PastRateActualSeconds = 1; }
        if (PastRateActualSeconds != 0 && PastRateTargetSeconds != 0) {
        PastRateAdjustmentRatio         = double(PastRateTargetSeconds) / double(PastRateActualSeconds);
        }
        EventHorizonDeviation           = 1 + (0.7084 * pow((double(PastBlocksMass)/double(9)), -1.228));
        EventHorizonDeviationFast       = EventHorizonDeviation;
        EventHorizonDeviationSlow       = 1 / EventHorizonDeviation;

        if (PastBlocksMass >= PastBlocksMin) {
            if ((PastRateAdjustmentRatio <= EventHorizonDeviationSlow) || (PastRateAdjustmentRatio >= EventHorizonDeviationFast)) { assert(BlockReading); break; }
        }
        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    BignumPtr bnNew = CopyBignum(PastDifficultyAverage.get());
    if (PastRateActualSeconds != 0 && PastRateTargetSeconds != 0) {
        BignumPtr actualSeconds = BignumFromUint64((uint64_t)PastRateActualSeconds);
        BignumPtr targetSeconds = BignumFromUint64((uint64_t)PastRateTargetSeconds);
        bnNew = MultiplyBignums(bnNew.get(), actualSeconds.get());
        bnNew = DivideBignums(bnNew.get(), targetSeconds.get());
    }
    if (BN_cmp(bnNew.get(), bnProofOfWorkLimit.get()) > 0) { bnNew = CopyBignum(bnProofOfWorkLimit.get()); }

    LogPrintf("Difficulty Retarget - BOB's Wormh0le (KGW)\n");
    LogPrintf("PastRateAdjustmentRatio = %g\n", PastRateAdjustmentRatio);
    LogPrintf("Before: %08x  %s\n", BlockLastSolved->nBits, Uint256FromBignum(BignumFromCompact(BlockLastSolved->nBits).get()).ToString().c_str());
    LogPrintf("After:  %08x  %s\n", CompactFromBignum(bnNew.get()), Uint256FromBignum(bnNew.get()).ToString().c_str());

    return CompactFromBignum(bnNew.get());
}

} // namespace

unsigned int static GetNextWorkRequired_V2(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
    static const int64  BlocksTargetSpacing         = 10 * 60; // 10 minutes
    unsigned int        TimeDaySeconds              = 60 * 60 * 24;
    int64               PastSecondsMin              = TimeDaySeconds * 0.0625;
    int64               PastSecondsMax              = TimeDaySeconds * 1.75;
    uint64              PastBlocksMin               = PastSecondsMin / BlocksTargetSpacing;
    uint64              PastBlocksMax               = PastSecondsMax / BlocksTargetSpacing; 
    
    return KimotoGravityWell(pindexLast, pblock, BlocksTargetSpacing, PastBlocksMin, PastBlocksMax);
}

unsigned int GetNextWorkRequired_V1(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
    unsigned int nProofOfWorkLimit = Params().ProofOfWorkLimit().GetCompact();

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    // Only change once per interval
    if ((pindexLast->nHeight+1) % Params().Interval() != 0)
    {
        if (Params().AllowMinDifficultyBlocks())
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + Params().TargetSpacing()*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % Params().Interval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < Params().Interval()-1; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    LogPrintf("  nActualTimespan = %d  before bounds\n", nActualTimespan);
    if (nActualTimespan < Params().TargetTimespan()/4)
        nActualTimespan = Params().TargetTimespan()/4;
    if (nActualTimespan > Params().TargetTimespan()*4)
        nActualTimespan = Params().TargetTimespan()*4;

    // Retarget
    uint256 bnNew;
    uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    bnNew *= nActualTimespan;
    bnNew /= Params().TargetTimespan();

    if (bnNew > Params().ProofOfWorkLimit())
        bnNew = Params().ProofOfWorkLimit();

    /// debug print
    LogPrintf("GetNextWorkRequired RETARGET\n");
    LogPrintf("Params().TargetTimespan() = %d    nActualTimespan = %d\n", Params().TargetTimespan(), nActualTimespan);
    LogPrintf("Before: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
    LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());

    return bnNew.GetCompact();
}

//digishield
unsigned int GetNextWorkRequired_V3(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{

    static const int64_t nTargetTimespan_V3 = 10*60 ; // dobbscoin: every 10 minutes
    static const int64_t nTargetSpacing_V3 = 10*60; // dobbscoin: 10 minutes
    static const int64_t nInterval_V3 = nTargetTimespan_V3 / nTargetSpacing_V3;

    unsigned int nProofOfWorkLimit = Params().ProofOfWorkLimit().GetCompact();

    
    int64_t retargetTimespan = nTargetTimespan_V3; //Params().TargetTimespan();
    int64_t retargetInterval = nInterval_V3; //Params().Interval();
    
    //retargetInterval = Params().TargetTimespan() / Params().TargetSpacing();
    //retargetTimespan = Params().TargetTimespan();
    
    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    // Only change once per interval
    if ((pindexLast->nHeight+1) % retargetInterval != 0)
    {
        if (Params().AllowMinDifficultyBlocks())
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* nTargetSpacing minutes
            // then allow mining of a min-difficulty block.
            if (pblock->nTime > pindexLast->nTime + nTargetSpacing_V3 * 30) //Params().TargetSpacing()*30)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % retargetInterval != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Fractalcoin: This fixes an issue where a 51% attack can change difficulty at will.
    // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
    int blockstogoback = retargetInterval-1;
    if ((pindexLast->nHeight+1) != retargetInterval)
        blockstogoback = retargetInterval;

    // Go back by what we want to be 14 days worth of blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < blockstogoback; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    LogPrintf("  nActualTimespan = %d  before bounds\n", nActualTimespan);
    
    //DigiShield implementation - thanks to RealSolid & WDC for this code
    // amplitude filter - thanks to daft27 for this code
    nActualTimespan = retargetTimespan + (nActualTimespan - retargetTimespan)/8;

    if (nActualTimespan < (retargetTimespan - (retargetTimespan/4)) ) nActualTimespan = (retargetTimespan - (retargetTimespan/4));
    if (nActualTimespan > (retargetTimespan + (retargetTimespan/2)) ) nActualTimespan = (retargetTimespan + (retargetTimespan/2));

    // Retarget
    uint256 bnNew;
    uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    //scale up for millisecond granularity
    bnNew *= nActualTimespan;
    //slingshield effectively works by making the target block time longer temporarily
    bnNew /= (retargetTimespan); 



    if (bnNew > Params().ProofOfWorkLimit())
        bnNew = Params().ProofOfWorkLimit();

    /// debug print
    LogPrintf("GetNextWorkRequired DIGISHIELD RETARGET\n");
    LogPrintf("Params().TargetTimespan() = %d    nActualTimespan = %d\n", Params().TargetTimespan(), nActualTimespan);
    LogPrintf("Before: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
    LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());
    return bnNew.GetCompact();
}

//normal digishield, but with 1 minute blocks
unsigned int GetNextWorkRequired_V4(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{

    static const int64_t nTargetTimespan_V4 = 2*60 ; // dobbscoin: every 2 minute
    static const int64_t nTargetSpacing_V4 = 2*60; // dobbscoin: 2 minute
    static const int64_t nInterval_V4 = nTargetTimespan_V4 / nTargetSpacing_V4;

    unsigned int nProofOfWorkLimit = Params().ProofOfWorkLimit().GetCompact();

    
    int64_t retargetTimespan = nTargetTimespan_V4; //Params().TargetTimespan();
    int64_t retargetInterval = nInterval_V4; //Params().Interval();
    
    //retargetInterval = Params().TargetTimespan() / Params().TargetSpacing();
    //retargetTimespan = Params().TargetTimespan();
    
    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    // Only change once per interval
    if ((pindexLast->nHeight+1) % retargetInterval != 0)
    {
        if (Params().AllowMinDifficultyBlocks())
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* nTargetSpacing minutes
            // then allow mining of a min-difficulty block.
            if (pblock->nTime > pindexLast->nTime + nTargetSpacing_V4 * 30) //Params().TargetSpacing()*30)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % retargetInterval != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // This fixes an issue where a 51% attack can change difficulty at will.
    // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
    int blockstogoback = retargetInterval-1;
    if ((pindexLast->nHeight+1) != retargetInterval)
        blockstogoback = retargetInterval;

    // Go back by what we want to be 14 days worth of blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < blockstogoback; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    LogPrintf("  nActualTimespan = %d  before bounds\n", nActualTimespan);
    
    //DigiShield implementation - thanks to RealSolid & WDC for this code
    // amplitude filter - thanks to daft27 for this code
    nActualTimespan = retargetTimespan + (nActualTimespan - retargetTimespan)/8;

    if (nActualTimespan < (retargetTimespan - (retargetTimespan/4)) ) nActualTimespan = (retargetTimespan - (retargetTimespan/4));
    if (nActualTimespan > (retargetTimespan + (retargetTimespan/2)) ) nActualTimespan = (retargetTimespan + (retargetTimespan/2));

    // Retarget
    uint256 bnNew;
    uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    //scale up for millisecond granularity
    bnNew *= nActualTimespan;
    bnNew /= (retargetTimespan); 



    if (bnNew > Params().ProofOfWorkLimit())
        bnNew = Params().ProofOfWorkLimit();

    /// debug print
    LogPrintf("GetNextWorkRequired DIGISHIELD RETARGET 2 MINUTES\n");
    LogPrintf("Params().TargetTimespan() = %d    nActualTimespan = %d\n", Params().TargetTimespan(), nActualTimespan);
    LogPrintf("Before: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
    LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());
    return bnNew.GetCompact();
}

// LWMA-3 (Zawy) — per-block weighted moving average retarget.
// Designed for low-hash PoW chains under NiceHash-rented attacks: most-recent
// block gets the highest weight, so difficulty tracks bursts of borrowed hash
// down within ~N blocks of the renter leaving instead of the multi-hour lag
// DigiShield exhibits on a 2-min target.
//
// Math invariants (justify the uint256 ordering below — non-obvious):
//   sumTarget   = Σ_{i=1..N} target_i / (N · k)     keeps each term ≤ ~2^212
//   weightedTime= Σ_{i=1..N} solvetime_i · i        bounded by 6T·N·(N+1)/2
//   nextTarget  = sumTarget · weightedTime          ≤ ~2^240  (safe in uint256)
// When solvetimes average exactly T, weightedTime == k and nextTarget collapses
// to the running average of recent targets — i.e. difficulty holds steady.
unsigned int GetNextWorkRequired_LWMA3(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
    enum { N = 60 };                                   // LWMA window (blocks)
    const int64_t T = 2 * 60;                          // target spacing (s)
    const int64_t k = (int64_t(N) * (N + 1) * T) / 2;  // expected Σ(i·T)
    const uint256 powLimit = Params().ProofOfWorkLimit();

    if (pindexLast == NULL)
        return powLimit.GetCompact();

    // Bootstrap window: until N blocks past the fork height we don't have
    // enough LWMA history, so re-use the previous block's bits. This means
    // the first N post-fork blocks inherit DigiShield's last value, then
    // LWMA-3 takes over cleanly.
    if (pindexLast->nHeight + 1 < LWMA3ForkHeight() + N)
        return pindexLast->nBits;

    // Walk back N+1 indices (we need N solvetimes).
    const CBlockIndex *blocks[N + 1];
    blocks[N] = pindexLast;
    for (int i = N - 1; i >= 0; --i)
        blocks[i] = blocks[i + 1]->pprev;

    uint256 sumTarget;
    int64_t weightedTime = 0;
    int64_t previousTime = blocks[0]->GetBlockTime();

    for (int64_t i = 1; i <= N; ++i) {
        int64_t thisTime = blocks[i]->GetBlockTime();
        int64_t solvetime = thisTime - previousTime;
        // Clamp solvetimes to [-6T, +6T] so a single bogus timestamp can't
        // dominate the average. Negative is allowed (timestamps can move
        // backward up to MTP) but capped symmetrically.
        if (solvetime >  6 * T) solvetime =  6 * T;
        if (solvetime < -6 * T) solvetime = -6 * T;
        previousTime = thisTime;

        weightedTime += solvetime * i;                 // weight = position i

        uint256 target;
        target.SetCompact(blocks[i]->nBits);
        sumTarget += target / (uint64_t)(N * k);       // divide-before-add
    }

    // Floor the weighted sum at k/3 so an extreme inbound burst can't push
    // next-target below ~1/3 of average (i.e. diff can at most ~3× per cycle).
    if (weightedTime < k / 3) weightedTime = k / 3;

    uint256 nextTarget = sumTarget * (uint64_t)weightedTime;
    if (nextTarget > powLimit) nextTarget = powLimit;

    LogPrintf("GetNextWorkRequired LWMA3 RETARGET  N=%d T=%d weightedTime=%d k=%d\n",
              (int)N, (int)T, (int)weightedTime, (int)k);
    LogPrintf("Before: %08x\n", pindexLast->nBits);
    LogPrintf("After:  %08x  %s\n", nextTarget.GetCompact(), nextTarget.ToString());
    return nextTarget.GetCompact();
}

int64_t LWMA3ForkHeight()
{
    return Params().AllowMinDifficultyBlocks() ? HARDFORK_LWMA3_TESTNET
                                               : HARDFORK_LWMA3_MAIN;
}

int AuxPowForkHeight()
{
    return Params().AllowMinDifficultyBlocks() ? HARDFORK_AUXPOW_TESTNET
                                               : HARDFORK_AUXPOW_MAIN;
}

int EmergencyDiffForkHeight()
{
    return Params().AllowMinDifficultyBlocks() ? HARDFORK_EMERGENCY_DIFF_TESTNET
                                               : HARDFORK_EMERGENCY_DIFF_MAIN;
}

bool IsEmergencyDifficultyBlock(const CBlockHeader& block, const CBlockIndex* pindexPrev)
{
    if (pindexPrev == NULL)
        return false;

    // Activation gate: pre-fork blocks can never invoke this relaxation.
    if (pindexPrev->nHeight + 1 < EmergencyDiffForkHeight())
        return false;

    // (b) Strict-greater on the timestamp gap. Equality at exactly 6h is not
    // sufficient — the rule fires only when the chain is genuinely stuck.
    // nTime is unsigned; subtract in signed space to handle moderate clock
    // skew (a header timestamp slightly behind prev) without underflow.
    const int64_t gap = (int64_t)block.nTime - (int64_t)pindexPrev->nTime;
    if (gap <= EMERGENCY_DIFFICULTY_GAP)
        return false;

    // (c) Min-difficulty only. A miner who can do better than powLimit after
    // the stall must publish at that better target — claiming min-difficulty
    // when you don't need it is rejected.
    if (block.nBits != Params().ProofOfWorkLimit().GetCompact())
        return false;

    return true;
}

bool CheckAuxPowProofOfWork(const CBlockHeader& block)
{
    if (block.IsAuxpow()) {
        if (!block.auxpow)
            return error("CheckAuxPowProofOfWork: VERSION_AUXPOW set but no auxpow attached");
        if (!block.auxpow->check(block.GetHash(), AUXPOW_CHAIN_ID))
            return error("CheckAuxPowProofOfWork: auxpow commitment chain failed");
        return CheckProofOfWork(block.auxpow->getParentBlockPoWHash(), block.nBits);
    }
    return CheckProofOfWork(block.GetPoWHash(), block.nBits);
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
    int DiffMode = 1;
    if (Params().AllowMinDifficultyBlocks()) {
        if (pindexLast->nHeight+1 >= 20) { DiffMode = 2; }
        if (pindexLast->nHeight+1 >= 50) { DiffMode = 3; }
        if(pindexLast->nHeight+1 >= 200) { DiffMode = 4; }
        if(pindexLast->nHeight+1 >= HARDFORK_LWMA3_TESTNET) { DiffMode = 5; }
    }
    else
    {
        if (pindexLast->nHeight+1 >= 13579) { DiffMode = 2; } // KGW @ Block 13579
        if(pindexLast->nHeight+1 >= 31597) { DiffMode = 3; } //switch to dobbshield @ block 31597
        if(pindexLast->nHeight+1 >= 68425) {DiffMode = 4; } //switch to 2 minute blocks with dobbshield
        if(pindexLast->nHeight+1 >= HARDFORK_LWMA3_MAIN) { DiffMode = 5; } // LWMA-3 + max-reorg fork
    }
    if      (DiffMode == 1) { return GetNextWorkRequired_V1(pindexLast, pblock); }
    else if (DiffMode == 2) { return GetNextWorkRequired_V2(pindexLast, pblock); }
    else if (DiffMode == 3) { return GetNextWorkRequired_V3(pindexLast, pblock); }
    else if (DiffMode == 4) { return GetNextWorkRequired_V4(pindexLast, pblock); }
    else if (DiffMode == 5) { return GetNextWorkRequired_LWMA3(pindexLast, pblock); }
    return GetNextWorkRequired_V3(pindexLast, pblock); //never reached..
}


bool CheckProofOfWork(uint256 hash, unsigned int nBits)
{
    bool fNegative;
    bool fOverflow;
    uint256 bnTarget;

    if(hash==HASHGENESISBLOCKPOW) //ignore genesis block
    {
        return true;
    }

    if (Params().SkipProofOfWorkCheck())
       return true;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > Params().ProofOfWorkLimit())
        return error("CheckProofOfWork() : nBits below minimum work");

    // Check proof of work matches claimed amount
    if (hash > bnTarget)
        return error("CheckProofOfWork() : hash doesn't match nBits");

    return true;
}

uint256 GetBlockProof(const CBlockIndex& block)
{
    uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for a uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (nTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}
