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

// AuxPoW (merge-mining) hard-fork activation heights.
// Mainnet: block 2,000,000 (~2026-12-17 at 2-min spacing), ~5 months after
// LWMA-3 settles. From this height: header chain ID must equal
// AUXPOW_CHAIN_ID; pre-fork the chain ID must be zero and the
// VERSION_AUXPOW bit must be unset.
// Testnet/regtest: trip at block 10 — before LWMA-3's testnet activation
// at 100, so regtest CPU mining stays at V1 difficulty and finishes quickly
// when test harnesses generate blocks past the fork.
static const int HARDFORK_AUXPOW_MAIN    = 2000000;
static const int HARDFORK_AUXPOW_TESTNET = 10;

// (BOB)'s AuxPoW chain ID, encoded in the upper 16 bits of nVersion on
// post-fork blocks. 0x00B0 — "B0b" mnemonic, picked for non-collision
// with active scrypt merge-mined chains (Doge=0x0062, Syscoin=0x0010,
// LottoCoin=0x004C, etc).
static const int AUXPOW_CHAIN_ID = 0x00B0;

int64_t LWMA3ForkHeight();
int AuxPowForkHeight();

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock);

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits);

/**
 * AuxPoW-aware proof-of-work check.  If the header carries an AuxPoW
 * commitment (VERSION_AUXPOW bit set in nVersion), validates the commitment
 * chain via CAuxPow::check() and tests the *parent* block's scrypt hash
 * against nBits.  Otherwise reduces to a normal CheckProofOfWork on the
 * header's own scrypt hash.  Caller is responsible for enforcing the
 * height-dependent rules around chain ID and the VERSION_AUXPOW bit.
 */
bool CheckAuxPowProofOfWork(const CBlockHeader& block);

uint256 GetBlockProof(const CBlockIndex& block);

#endif // DOBBSCOIN_POW_H
