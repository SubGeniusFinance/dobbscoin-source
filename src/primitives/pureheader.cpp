// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2024 The Dogecoin Core developers
// Copyright (c) 2026 The Dobbscoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/pureheader.h"

#include "hash.h"
#include "utilstrencodings.h"
#include "crypto/scrypt.h"

#include <cassert>

void CPureBlockHeader::SetBaseVersion(int32_t nBaseVersion, int32_t nChainId)
{
    assert(nBaseVersion >= 1 && nBaseVersion < VERSION_AUXPOW);
    assert(!IsAuxpow());
    nVersion = nBaseVersion | (nChainId * VERSION_CHAIN_START);
}

uint256 CPureBlockHeader::GetHash() const
{
    return Hash(BEGIN(nVersion), END(nNonce));
}

uint256 CPureBlockHeader::GetPoWHash() const
{
    return HashScrypt(BEGIN(nVersion), END(nNonce));
}
