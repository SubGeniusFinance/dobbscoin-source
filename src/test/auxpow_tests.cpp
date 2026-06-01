// Copyright (c) 2014-2015 Daniel Kraft
// Copyright (c) 2026 The Dobbscoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "auxpow.h"
#include "chainparams.h"
#include "chainparamsbase.h"
#include "main.h"
#include "pow.h"
#include "primitives/block.h"
#include "script/script.h"
#include "uint256.h"
#include "utilstrencodings.h"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <vector>

BOOST_AUTO_TEST_SUITE(auxpow_tests)

namespace {

// Tamper with a uint256 by incrementing it by 1.
static void tamperWith(uint256& num)
{
    num += 1;
}

/**
 * Utility class that builds an AuxPoW structure step-by-step.  Mirrors the
 * shape of Doge's CAuxpowBuilder but adapted to (BOB)'s value-type
 * CTransaction and CBlock::BuildMerkleTree instead of CTransactionRef and
 * BlockMerkleRoot.
 */
class CAuxpowBuilder
{
public:
    CBlock parentBlock;
    std::vector<uint256> auxpowChainMerkleBranch;
    int auxpowChainIndex;

    CAuxpowBuilder(int baseVersion, int chainId)
        : auxpowChainIndex(-1)
    {
        parentBlock.SetBaseVersion(baseVersion, chainId);
    }

    void setCoinbase(const CScript& scr)
    {
        CMutableTransaction mtx;
        mtx.vin.resize(1);
        mtx.vin[0].prevout.SetNull();
        mtx.vin[0].scriptSig = scr;

        parentBlock.vtx.clear();
        parentBlock.vtx.push_back(CTransaction(mtx));
        parentBlock.hashMerkleRoot = parentBlock.BuildMerkleTree();
    }

    std::vector<unsigned char>
    buildAuxpowChain(const uint256& hashAux, unsigned h, int index)
    {
        auxpowChainIndex = index;

        // Synthetic merkle branch — the exact values don't matter, only that
        // CheckMerkleBranch reproduces a consistent root.
        auxpowChainMerkleBranch.clear();
        for (unsigned i = 0; i < h; ++i)
            auxpowChainMerkleBranch.push_back(uint256(i));

        const uint256 hash = CAuxPow::CheckMerkleBranch(hashAux, auxpowChainMerkleBranch, index);

        std::vector<unsigned char> res = ToByteVector(hash);
        std::reverse(res.begin(), res.end());
        return res;
    }

    CAuxPow get(const CTransaction& tx) const
    {
        CAuxPow res(tx);
        // Inline equivalent of CMerkleTx::InitMerkleBranch for index 0
        // (parent coinbase is always at index 0 in our construction).
        res.hashBlock = parentBlock.GetHash();
        res.vMerkleBranch = parentBlock.GetMerkleBranch(0);
        res.nIndex = 0;
        res.vChainMerkleBranch = auxpowChainMerkleBranch;
        res.nChainIndex = auxpowChainIndex;
        res.parentBlock = parentBlock;
        return res;
    }

    CAuxPow get() const
    {
        assert(!parentBlock.vtx.empty());
        return get(parentBlock.vtx[0]);
    }

    /**
     * Build a data vector to be embedded in the parent coinbase scriptSig:
     * optional fabe-mm magic, then the chain merkle root, the merkle tree
     * size (1 << h), and the nonce.
     */
    static std::vector<unsigned char>
    buildCoinbaseData(bool header, const std::vector<unsigned char>& auxRoot, unsigned h, int nonce)
    {
        std::vector<unsigned char> res;
        if (header)
            res.insert(res.end(), UBEGIN(pchMergedMiningHeader), UEND(pchMergedMiningHeader));
        res.insert(res.end(), auxRoot.begin(), auxRoot.end());

        const int size = (1 << h);
        res.insert(res.end(), UBEGIN(size), UEND(size));
        res.insert(res.end(), UBEGIN(nonce), UEND(nonce));
        return res;
    }
};

// Mine a header against `nBits` (or its own nBits if -1) by incrementing
// nNonce until the PoW hash falls below (or above, when ok=false) the target.
static void mineBlock(CBlockHeader& block, bool ok, int nBits = -1)
{
    if (nBits == -1)
        nBits = block.nBits;

    uint256 target;
    target.SetCompact(nBits);

    block.nNonce = 0;
    while (true) {
        const bool nowOk = (block.GetPoWHash() <= target);
        if ((ok && nowOk) || (!ok && !nowOk))
            break;
        ++block.nNonce;
    }

    if (ok)
        BOOST_CHECK(CheckProofOfWork(block.GetPoWHash(), nBits));
    else
        BOOST_CHECK(!CheckProofOfWork(block.GetPoWHash(), nBits));
}

} // anon

/* ************************************************************************** */

BOOST_AUTO_TEST_CASE(check_auxpow_merkle_commitment)
{
    // Use chain ID 42 for the parent — any value != AUXPOW_CHAIN_ID works,
    // since (BOB)'s rule rejects parents whose chain ID matches ours.
    const int parentChainId = 42;
    const int32_t ourChainId = AUXPOW_CHAIN_ID;

    CAuxpowBuilder builder(5, parentChainId);
    CAuxPow auxpow;

    const uint256 hashAux(12345);
    const unsigned height = 30;
    const int nonce = 7;
    int index;

    std::vector<unsigned char> auxRoot, data;
    CScript scr;

    // Build a correct auxpow at maximum allowed merkle height.
    index = CAuxPow::getExpectedIndex(nonce, ourChainId, height);
    auxRoot = builder.buildAuxpowChain(hashAux, height, index);
    data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height, nonce);
    scr = (CScript() << 2809 << 2013) + COINBASE_FLAGS;
    scr = (scr << OP_2 << data);
    builder.setCoinbase(scr);
    BOOST_CHECK(builder.get().check(hashAux, ourChainId));

    // Parent coinbase with no inputs → reject.
    CMutableTransaction mtx(builder.parentBlock.vtx[0]);
    mtx.vin.clear();
    builder.parentBlock.vtx.clear();
    builder.parentBlock.vtx.push_back(CTransaction(mtx));
    builder.parentBlock.hashMerkleRoot = builder.parentBlock.BuildMerkleTree();
    BOOST_CHECK(!builder.get().check(hashAux, ourChainId));

    // Tamper with the aux hash or chain ID → reject.
    uint256 modifiedAux(hashAux);
    tamperWith(modifiedAux);
    BOOST_CHECK(!builder.get().check(modifiedAux, ourChainId));
    BOOST_CHECK(!builder.get().check(hashAux, ourChainId + 1));

    // Restore a valid build.
    builder = CAuxpowBuilder(5, parentChainId);
    index = CAuxPow::getExpectedIndex(nonce, ourChainId, height);
    auxRoot = builder.buildAuxpowChain(hashAux, height, index);
    data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height, nonce);
    scr = (CScript() << 2809 << 2013) + COINBASE_FLAGS;
    scr = (scr << OP_2 << data);
    builder.setCoinbase(scr);

    // Non-coinbase parent tx (vtx[1] instead of [0]) → reject.
    const CTransaction oldCoinbase = builder.parentBlock.vtx[0];
    builder.setCoinbase(scr << 5);
    builder.parentBlock.vtx.push_back(oldCoinbase);
    builder.parentBlock.hashMerkleRoot = builder.parentBlock.BuildMerkleTree();
    auxpow = builder.get(builder.parentBlock.vtx[0]);
    BOOST_CHECK(auxpow.check(hashAux, ourChainId));
    auxpow = builder.get(builder.parentBlock.vtx[1]);
    BOOST_CHECK(!auxpow.check(hashAux, ourChainId));

    // Parent chain ID == ours → reject (strict chain ID enforcement).
    CAuxpowBuilder bSame = builder;
    bSame.parentBlock.SetChainId(100);
    BOOST_CHECK(bSame.get().check(hashAux, ourChainId));
    bSame.parentBlock.SetChainId(ourChainId);
    BOOST_CHECK(!bSame.get().check(hashAux, ourChainId));

    // Merkle branch longer than 30 → reject.
    CAuxpowBuilder bLong = builder;
    int longIndex = CAuxPow::getExpectedIndex(nonce, ourChainId, height + 1);
    auxRoot = bLong.buildAuxpowChain(hashAux, height + 1, longIndex);
    data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height + 1, nonce);
    scr = (CScript() << 2809 << 2013) + COINBASE_FLAGS;
    scr = (scr << OP_2 << data);
    bLong.setCoinbase(scr);
    BOOST_CHECK(!bLong.get().check(hashAux, ourChainId));

    // Tampered parent merkle root → reject.
    CAuxpowBuilder bMutated = builder;
    BOOST_CHECK(bMutated.get().check(hashAux, ourChainId));
    tamperWith(bMutated.parentBlock.hashMerkleRoot);
    BOOST_CHECK(!bMutated.get().check(hashAux, ourChainId));
}

BOOST_AUTO_TEST_CASE(check_auxpow_legacy_no_magic)
{
    // No fabe-mm magic: root must live in the first 20 bytes of the coinbase.
    const int parentChainId = 42;
    const int32_t ourChainId = AUXPOW_CHAIN_ID;
    const unsigned height = 3;
    const int nonce = 7;
    const int index = CAuxPow::getExpectedIndex(nonce, ourChainId, height);

    CAuxpowBuilder builder(5, parentChainId);
    std::vector<unsigned char> auxRoot = builder.buildAuxpowChain(uint256(12345), height, index);
    std::vector<unsigned char> data = CAuxpowBuilder::buildCoinbaseData(false, auxRoot, height, nonce);
    builder.setCoinbase(CScript() << data);
    BOOST_CHECK(builder.get().check(uint256(12345), ourChainId));

    // Two roots embedded — disallowed when the fabe-mm magic is present.
    std::vector<unsigned char> wrong = builder.buildAuxpowChain(uint256(99999), height, index);
    auxRoot = builder.buildAuxpowChain(uint256(12345), height, index);
    std::vector<unsigned char> dataMagic = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height, nonce);
    std::vector<unsigned char> wrongMagic = CAuxpowBuilder::buildCoinbaseData(true, wrong, height, nonce);
    builder.setCoinbase(CScript() << dataMagic << wrongMagic);
    BOOST_CHECK(!builder.get().check(uint256(12345), ourChainId));
}

BOOST_AUTO_TEST_CASE(check_auxpow_nonce_and_size_tampering)
{
    const int parentChainId = 42;
    const int32_t ourChainId = AUXPOW_CHAIN_ID;
    const unsigned height = 3;
    const int nonce = 7;
    const int index = CAuxPow::getExpectedIndex(nonce, ourChainId, height);
    CAuxpowBuilder builder(5, parentChainId);
    std::vector<unsigned char> auxRoot = builder.buildAuxpowChain(uint256(12345), height, index);

    // Baseline: valid.
    std::vector<unsigned char> data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height, nonce);
    builder.setCoinbase(CScript() << data);
    BOOST_CHECK(builder.get().check(uint256(12345), ourChainId));

    // Truncate one byte → missing tail data, reject.
    data.pop_back();
    builder.setCoinbase(CScript() << data);
    BOOST_CHECK(!builder.get().check(uint256(12345), ourChainId));

    // Wrong merkle-tree size in the commitment → reject.
    data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height - 1, nonce);
    builder.setCoinbase(CScript() << data);
    BOOST_CHECK(!builder.get().check(uint256(12345), ourChainId));

    // Wrong nonce → expected index drifts, reject.
    data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height, nonce + 3);
    builder.setCoinbase(CScript() << data);
    BOOST_CHECK(!builder.get().check(uint256(12345), ourChainId));
}

/* ************************************************************************** */

// CheckAuxPowProofOfWork exerciser.  Three lanes:
//   1. Non-AuxPoW header — falls through to standard scrypt check.
//   2. AuxPoW header with valid commitment and a parent that meets target.
//   3. AuxPoW header with valid commitment but parent below target → fail.
//   4. AuxPoW header whose (BOB) merkle root is tampered → commitment fails.
BOOST_AUTO_TEST_CASE(check_auxpow_pow_helper)
{
    // Regtest: ProofOfWorkLimit() = ~0 >> 1, so we can mine at maximum target
    // in a couple of iterations.  CheckProofOfWork rejects any target above
    // ProofOfWorkLimit, so without this we'd fail under mainnet defaults.
    SelectParams(CBaseChainParams::REGTEST);

    const uint256 target = (~uint256(0)) >> 1;
    CBlockHeader block;
    block.nBits = target.GetCompact();

    // Lane 1: non-AuxPoW header, pure scrypt check.
    block.SetChainId(0);
    block.SetAuxpowFlag(false);
    mineBlock(block, true);
    BOOST_CHECK(CheckAuxPowProofOfWork(block));
    mineBlock(block, false);
    BOOST_CHECK(!CheckAuxPowProofOfWork(block));

    // Lane 2: AuxPoW header with a valid commitment + parent meeting target.
    block.SetChainId(AUXPOW_CHAIN_ID);
    block.SetAuxpowFlag(true);

    const int parentChainId = 42;
    const unsigned height = 3;
    const int nonce = 7;
    const int index = CAuxPow::getExpectedIndex(nonce, AUXPOW_CHAIN_ID, height);

    CAuxpowBuilder builder(5, parentChainId);
    std::vector<unsigned char> auxRoot = builder.buildAuxpowChain(block.GetHash(), height, index);
    std::vector<unsigned char> data = CAuxpowBuilder::buildCoinbaseData(true, auxRoot, height, nonce);
    builder.setCoinbase(CScript() << data);
    builder.parentBlock.nBits = block.nBits;

    // Lane 3: parent below target → CheckProofOfWork on parent fails.
    mineBlock(builder.parentBlock, false, block.nBits);
    block.SetAuxpow(new CAuxPow(builder.get()));
    BOOST_CHECK(!CheckAuxPowProofOfWork(block));

    // Lane 2 proper: parent meets target → full path passes.
    mineBlock(builder.parentBlock, true, block.nBits);
    block.SetAuxpow(new CAuxPow(builder.get()));
    BOOST_CHECK(CheckAuxPowProofOfWork(block));

    // Lane 4: tamper with (BOB) merkle root after the commitment was built.
    // The aux block hash drifts; commitment chain no longer recovers the
    // root embedded in the parent coinbase → reject.
    tamperWith(block.hashMerkleRoot);
    BOOST_CHECK(!CheckAuxPowProofOfWork(block));

    // Restore mainnet so subsequent test cases (transaction_tests/tx_valid
    // in particular) don't see leaked REGTEST consensus parameters.
    SelectParams(CBaseChainParams::MAIN);
}

BOOST_AUTO_TEST_SUITE_END()
