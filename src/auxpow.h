// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2016 Daniel Kraft
// Copyright (c) 2026 The Dobbscoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DOBBSCOIN_AUXPOW_H
#define DOBBSCOIN_AUXPOW_H

#include "primitives/pureheader.h"
#include "primitives/transaction.h"
#include "serialize.h"
#include "uint256.h"

#include <vector>

/** Header magic prefix for the AuxPoW commitment inside a parent block's
 *  coinbase scriptSig.  See CAuxPow::check(). */
static const unsigned char pchMergedMiningHeader[] = { 0xfa, 0xbe, 'm', 'm' };

/**
 * AuxPoW (merge-mining) container.  Carries a parent-chain block header (on
 * which the actual scrypt PoW was solved) plus the merkle proofs needed to
 * show that this (BOB) block's hash is committed inside the parent's coinbase.
 *
 * Wire layout deliberately mirrors Namecoin/Dogecoin: a CMerkleTx-shaped
 * prefix (parent coinbase tx + parent block hash + merkle branch + index)
 * followed by the AuxPoW-specific fields (chain merkle branch + chain index
 * + parent header).
 *
 * (BOB)'s existing CMerkleTx lives in wallet.h and we don't want CAuxPow to
 * drag the wallet into consensus-side code paths, so the CMerkleTx fields
 * are inlined here as direct members instead of inherited.  The byte layout
 * is identical regardless.
 */
class CAuxPow
{
public:
    /* CMerkleTx-shaped prefix.  Together these prove parent.tx is in
       parent.hashBlock's merkle tree at index nIndex. */
    CTransaction tx;
    uint256 hashBlock;
    std::vector<uint256> vMerkleBranch;
    int nIndex;

    /* AuxPoW-specific. */
    std::vector<uint256> vChainMerkleBranch;
    int nChainIndex;
    CPureBlockHeader parentBlock;

    CAuxPow()
        : nIndex(0), nChainIndex(0)
    {
        hashBlock = 0;
    }

    explicit CAuxPow(const CTransaction& txIn)
        : tx(txIn), nIndex(0), nChainIndex(0)
    {
        hashBlock = 0;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(tx);
        nVersion = tx.nVersion;
        READWRITE(hashBlock);
        READWRITE(vMerkleBranch);
        READWRITE(nIndex);
        READWRITE(vChainMerkleBranch);
        READWRITE(nChainIndex);
        READWRITE(parentBlock);
    }

    /**
     * Validate the AuxPoW commitment chain.  Does NOT verify that the parent
     * block's scrypt PoW meets (BOB)'s target — that is the caller's job via
     * the usual CheckProofOfWork path against getParentBlockPoWHash().
     *
     * Enforces strict chain ID separation: the parent block must declare a
     * chain ID different from (BOB)'s, so a (BOB) block cannot itself serve
     * as a parent in another (BOB) block's AuxPoW.
     *
     * @param hashAuxBlock  Hash of this (BOB) block (the merge-mined block).
     * @param nChainId      (BOB)'s chain ID (AUXPOW_CHAIN_ID, 0x00B0).
     * @return True if the merkle proofs and the coinbase commitment all check.
     */
    bool check(const uint256& hashAuxBlock, int nChainId) const;

    /** Parent block's scrypt PoW hash, to be compared against (BOB)'s nBits. */
    uint256 getParentBlockPoWHash() const { return parentBlock.GetPoWHash(); }

    /** Parent block accessor (used by the RPC layer and tests). */
    const CPureBlockHeader& getParentBlock() const { return parentBlock; }

    /**
     * Deterministic merkle-leaf index for a (chainId, nonce, height) triple.
     * Both miner and validator compute this so a parent coinbase cannot reuse
     * the same slot for two different aux chains.
     */
    static int getExpectedIndex(uint32_t nNonce, int nChainId, unsigned h);

    /** Standalone CheckMerkleBranch — same algorithm as CBlock's, kept here
     *  so CAuxPow does not need to depend on CBlock. */
    static uint256 CheckMerkleBranch(uint256 hash,
                                     const std::vector<uint256>& vMerkleBranch,
                                     int nIndex);
};

#endif // DOBBSCOIN_AUXPOW_H
