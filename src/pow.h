// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DOBBSCOIN_POW_H
#define DOBBSCOIN_POW_H

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class uint256;

// LWMA-3 hard-fork activation heights (DiffMode V5).
// Mainnet: block 1,888,808 — ~54-day upgrade window from 2026-05-22 at
// 2-min target spacing (~2026-07-16). The 808 suffix is intentional
// SubGenius-friendly vanity; the height is a future block, not a date.
// Testnet/regtest: trip immediately so test infra exercises the new path.
static const int64_t HARDFORK_LWMA3_MAIN    = 1888808;
static const int64_t HARDFORK_LWMA3_TESTNET = 100;

// Consensus-level finality: a chain that would reorganize past this many
// already-buried blocks of the active tip is rejected outright. 100 blocks
// * 2 min = ~3.3h. The wBOB bridge requires 288 conf (~9.6h), so the bridge
// remains strictly more conservative than consensus — correct ordering.
static const int MAX_REORG_DEPTH = 100;

int64_t LWMA3ForkHeight();

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock);

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits);
uint256 GetBlockProof(const CBlockIndex& block);

#endif // DOBBSCOIN_POW_H
