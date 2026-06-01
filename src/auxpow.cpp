// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011 Vince Durham
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2016 Daniel Kraft
// Copyright (c) 2026 The Dobbscoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "auxpow.h"

#include "hash.h"
#include "script/script.h"
#include "util.h"
#include "utilstrencodings.h"

#include <algorithm>
#include <cstring>
#include <endian.h>

bool CAuxPow::check(const uint256& hashAuxBlock, int nChainId) const
{
    if (nIndex != 0)
        return error("AuxPow is not a generate");

    if (parentBlock.GetChainId() == nChainId)
        return error("AuxPow parent has our chain ID");

    if (vChainMerkleBranch.size() > 30)
        return error("AuxPow chain merkle branch too long");

    // Check that the chain merkle root hashes (after byte-reversal for the
    // big-endian representation used in script bytes) is committed in the
    // parent coinbase.
    const uint256 nRootHash = CheckMerkleBranch(hashAuxBlock, vChainMerkleBranch, nChainIndex);
    std::vector<unsigned char> vchRootHash(nRootHash.begin(), nRootHash.end());
    std::reverse(vchRootHash.begin(), vchRootHash.end());

    // Check the parent coinbase is in the parent block's merkle tree at nIndex.
    if (CheckMerkleBranch(tx.GetHash(), vMerkleBranch, nIndex) != parentBlock.hashMerkleRoot)
        return error("AuxPow merkle root incorrect");

    if (tx.vin.empty())
        return error("AuxPow coinbase has no inputs");

    const CScript script = tx.vin[0].scriptSig;

    CScript::const_iterator pcHead =
        std::search(script.begin(), script.end(), UBEGIN(pchMergedMiningHeader), UEND(pchMergedMiningHeader));

    CScript::const_iterator pc =
        std::search(script.begin(), script.end(), vchRootHash.begin(), vchRootHash.end());

    if (pc == script.end())
        return error("AuxPow missing chain merkle root in parent coinbase");

    if (pcHead != script.end()) {
        // The standard ("modern") path: fabe-mm magic followed immediately by
        // the chain merkle root.  Enforce single occurrence.
        if (script.end() != std::search(pcHead + 1, script.end(), UBEGIN(pchMergedMiningHeader), UEND(pchMergedMiningHeader)))
            return error("Multiple merged mining headers in coinbase");
        if (pcHead + sizeof(pchMergedMiningHeader) != pc)
            return error("Merged mining header is not just before chain merkle root");
    } else {
        // Backward-compatibility path: no magic; require the root to start
        // within the first 20 bytes so an attacker cannot stuff alternative
        // commitments earlier in the script.
        if (pc - script.begin() > 20)
            return error("AuxPow chain merkle root must start in the first 20 bytes of the parent coinbase");
    }

    // After the chain root, expect 4 bytes of tree size + 4 bytes of nonce.
    pc += vchRootHash.size();
    if (script.end() - pc < 8)
        return error("AuxPow missing chain merkle tree size and nonce in parent coinbase");

    uint32_t nSize;
    std::memcpy(&nSize, &pc[0], 4);
    nSize = le32toh(nSize);
    const unsigned merkleHeight = vChainMerkleBranch.size();
    if (nSize != (1u << merkleHeight))
        return error("AuxPow merkle branch size does not match parent coinbase");

    uint32_t nNonce;
    std::memcpy(&nNonce, &pc[4], 4);
    nNonce = le32toh(nNonce);
    if (nChainIndex != getExpectedIndex(nNonce, nChainId, merkleHeight))
        return error("AuxPow wrong index");

    return true;
}

int CAuxPow::getExpectedIndex(uint32_t nNonce, int nChainId, unsigned h)
{
    // Pseudo-random slot derived from (nonce, chainId, height).  The intent
    // is to make slot collisions between different aux chains unlikely while
    // still being deterministic for any given (chainId, nonce, height).
    //
    // Overflow in uint32 is fine — we take mod against a power-of-two at
    // the end, and h is capped at 30 elsewhere.
    uint32_t rand = nNonce;
    rand = rand * 1103515245 + 12345;
    rand += nChainId;
    rand = rand * 1103515245 + 12345;
    return rand % (1u << h);
}

uint256 CAuxPow::CheckMerkleBranch(uint256 hash,
                                   const std::vector<uint256>& vMerkleBranch,
                                   int nIndex)
{
    if (nIndex == -1)
        return 0;
    for (std::vector<uint256>::const_iterator it(vMerkleBranch.begin());
         it != vMerkleBranch.end(); ++it) {
        if (nIndex & 1)
            hash = Hash(BEGIN(*it), END(*it), BEGIN(hash), END(hash));
        else
            hash = Hash(BEGIN(hash), END(hash), BEGIN(*it), END(*it));
        nIndex >>= 1;
    }
    return hash;
}
