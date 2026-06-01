// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2024 The Dogecoin Core developers
// Copyright (c) 2026 The Dobbscoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DOBBSCOIN_PRIMITIVES_PUREHEADER_H
#define DOBBSCOIN_PRIMITIVES_PUREHEADER_H

#include "serialize.h"
#include "uint256.h"

/**
 * A block header without auxpow information. Splitting this out from
 * CBlockHeader breaks the cyclic dependency between auxpow (which references
 * a parent block header) and the block header (which optionally carries an
 * auxpow). The parent block header in an auxpow commitment is itself a pure
 * header.
 *
 * On (BOB), this is the 80-byte header that scrypt hashes. AuxPoW activates
 * at mainnet block HARDFORK_AUXPOW_MAIN; pre-fork, the auxpow bit and chain
 * ID are required to be zero.
 */
class CPureBlockHeader
{
public:
    /** Modifier bit set in nVersion when this header carries an AuxPoW. */
    static const int32_t VERSION_AUXPOW = (1 << 8);

    /** Upper 16 bits of nVersion encode the auxpow chain ID. */
    static const int32_t VERSION_CHAIN_START = (1 << 16);

    // header
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;

    CPureBlockHeader()
    {
        SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
    }

    void SetNull()
    {
        nVersion = 0;
        hashPrevBlock = 0;
        hashMerkleRoot = 0;
        nTime = 0;
        nBits = 0;
        nNonce = 0;
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }

    uint256 GetHash() const;
    uint256 GetPoWHash() const;

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }

    /* nVersion accessors split base version, chain ID, and auxpow flag.
       Layout: [chain_id : 16][reserved : 7][auxpow : 1][base_version : 8] */

    inline int32_t GetBaseVersion() const
    {
        return GetBaseVersion(nVersion);
    }
    static inline int32_t GetBaseVersion(int32_t ver)
    {
        return ver % VERSION_AUXPOW;
    }

    /**
     * Set the base version (lower 8 bits) and chain ID (upper 16 bits) at once.
     * Must only be called before any auxpow has been attached.
     */
    void SetBaseVersion(int32_t nBaseVersion, int32_t nChainId);

    inline int32_t GetChainId() const
    {
        return nVersion >> 16;
    }

    inline void SetChainId(int32_t chainId)
    {
        nVersion %= VERSION_CHAIN_START;
        nVersion |= chainId * VERSION_CHAIN_START;
    }

    inline bool IsAuxpow() const
    {
        return nVersion & VERSION_AUXPOW;
    }

    inline void SetAuxpowFlag(bool auxpow)
    {
        if (auxpow)
            nVersion |= VERSION_AUXPOW;
        else
            nVersion &= ~VERSION_AUXPOW;
    }

    /**
     * A "legacy" block is one minted before the AuxPoW hardfork: it has no
     * chain ID in nVersion and no auxpow trailer. (BOB) pre-fork blocks
     * (nVersion 1/2/3) all qualify.
     */
    inline bool IsLegacy() const
    {
        return GetChainId() == 0;
    }
};

#endif // DOBBSCOIN_PRIMITIVES_PUREHEADER_H
