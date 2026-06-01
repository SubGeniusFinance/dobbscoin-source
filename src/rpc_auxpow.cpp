// Copyright (c) 2009-2014 The Bitcoin Core developers
// Copyright (c) 2011 Vince Durham
// Copyright (c) 2014-2016 Daniel Kraft
// Copyright (c) 2026 The Dobbscoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "auxpow.h"
#include "base58.h"
#include "chainparams.h"
#include "main.h"
#include "miner.h"
#include "net.h"
#include "pow.h"
#include "primitives/block.h"
#include "rpcserver.h"
#include "script/standard.h"
#include "streams.h"
#include "sync.h"
#include "txmempool.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"
#include "version.h"

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace json_spirit;
using namespace std;

// Serialise access to the aux-block cache.
static CCriticalSection cs_auxpowrpc;

// Cache of unsolved templates handed out by createauxblock, keyed by the
// (post-VERSION_AUXPOW-flag) block hash so submitauxblock can locate them.
// Owned templates kept in vAuxTemplates to keep CBlockTemplate-managed
// state alive (vTxFees/vTxSigOps etc).
static std::map<uint256, std::shared_ptr<CBlock>> mapAuxBlocks;
static std::vector<std::unique_ptr<CBlockTemplate>> vAuxTemplates;
static CBlockIndex* pindexPrevAux = NULL;
static unsigned int nAuxExtraNonce = 0;
static int64_t nAuxStart = 0;
static unsigned int nAuxTransactionsUpdatedLast = 0;

static void AuxMiningCheck()
{
    {
        LOCK(cs_vNodes);
        if (vNodes.empty() && !Params().MineBlocksOnDemand())
            throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED,
                               "Dobbscoin is not connected");
    }

    if (IsInitialBlockDownload() && !Params().MineBlocksOnDemand())
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD,
                           "Dobbscoin is downloading blocks");

    LOCK(cs_main);
    if (chainActive.Height() + 1 < AuxPowForkHeight())
        throw JSONRPCError(RPC_METHOD_NOT_FOUND,
                           "AuxPoW activation height not yet reached");
}

static Object CreateAuxBlockHelper(const CScript& scriptPubKey)
{
    AuxMiningCheck();
    LOCK(cs_auxpowrpc);

    std::shared_ptr<CBlock> pblock;
    {
        LOCK(cs_main);

        const bool fStale =
            (pindexPrevAux != chainActive.Tip())
            || (mempool.GetTransactionsUpdated() != nAuxTransactionsUpdatedLast
                && GetTime() - nAuxStart > 60);
        if (fStale) {
            mapAuxBlocks.clear();
            vAuxTemplates.clear();
        }

        std::unique_ptr<CBlockTemplate> newTemplate(CreateNewBlock(scriptPubKey));
        if (!newTemplate)
            throw JSONRPCError(RPC_OUT_OF_MEMORY, "out of memory");

        nAuxTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
        pindexPrevAux = chainActive.Tip();
        nAuxStart = GetTime();

        // Finalise: stamp the coinbase extra-nonce, flip the AuxPoW bit, then
        // hash.  Chain ID is already in nVersion (set by CreateNewBlock's
        // post-fork hook in miner.cpp).
        IncrementExtraNonce(&newTemplate->block, pindexPrevAux, nAuxExtraNonce);
        newTemplate->block.SetAuxpowFlag(true);

        pblock = std::make_shared<CBlock>(newTemplate->block);
        mapAuxBlocks[pblock->GetHash()] = pblock;
        vAuxTemplates.push_back(std::move(newTemplate));
    }

    assert(pblock);

    uint256 target;
    bool fNegative = false, fOverflow = false;
    target.SetCompact(pblock->nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || target == 0)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "invalid difficulty bits in block");

    Object result;
    result.push_back(Pair("hash",              pblock->GetHash().GetHex()));
    result.push_back(Pair("chainid",           (int64_t)pblock->GetChainId()));
    result.push_back(Pair("previousblockhash", pblock->hashPrevBlock.GetHex()));
    result.push_back(Pair("coinbasevalue",     (int64_t)pblock->vtx[0].vout[0].nValue));
    result.push_back(Pair("bits",              strprintf("%08x", pblock->nBits)));
    result.push_back(Pair("height",            (int64_t)(pindexPrevAux->nHeight + 1)));
    result.push_back(Pair("target",            HexStr(BEGIN(target), END(target))));
    return result;
}

static bool DecodeAuxPow(CAuxPow& auxpow, const std::string& strHex)
{
    if (!IsHex(strHex))
        return false;
    std::vector<unsigned char> data = ParseHex(strHex);
    CDataStream ss(data, SER_NETWORK, PROTOCOL_VERSION);
    try {
        ss >> auxpow;
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

static bool SubmitAuxBlockHelper(const uint256& hash, const CAuxPow& auxpow)
{
    AuxMiningCheck();

    std::shared_ptr<CBlock> pblock;
    {
        LOCK(cs_auxpowrpc);
        std::map<uint256, std::shared_ptr<CBlock>>::iterator it = mapAuxBlocks.find(hash);
        if (it == mapAuxBlocks.end())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "block hash unknown");
        pblock = it->second;
    }

    CBlock& block = *pblock;
    block.SetAuxpow(new CAuxPow(auxpow));
    if (block.GetHash() != hash)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "cached block hash drifted under attached auxpow");

    CValidationState state;
    return ProcessNewBlock(state, NULL, &block, NULL);
}

Value createauxblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "createauxblock <address>\n"
            "\nCreate a new block and return information required to merge-mine it.\n"
            "\nArguments:\n"
            "1. address      (string, required) coinbase payout address\n"
            "\nResult:\n"
            "{\n"
            "  \"hash\"               (string)  hash of the created block\n"
            "  \"chainid\"            (numeric) (BOB)'s AuxPoW chain ID (0x00B0 = 176)\n"
            "  \"previousblockhash\"  (string)  hash of the previous block\n"
            "  \"coinbasevalue\"      (numeric) value of the coinbase output, satoshis\n"
            "  \"bits\"               (string)  compressed difficulty target\n"
            "  \"height\"             (numeric) height of the new block\n"
            "  \"target\"             (string)  target in reversed byte order, 64 hex chars\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("createauxblock", "\"BobAddress...\"")
            + HelpExampleRpc("createauxblock", "\"BobAddress...\"")
        );

    CDobbscoinAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid coinbase payout address");

    return CreateAuxBlockHelper(GetScriptForDestination(address.Get()));
}

Value submitauxblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "submitauxblock <hash> <auxpow>\n"
            "\nSubmit a solved auxpow for a previously created block.\n"
            "\nArguments:\n"
            "1. hash      (string, required) hash returned by createauxblock\n"
            "2. auxpow    (string, required) hex-encoded serialized CAuxPow\n"
            "\nResult:\n"
            "true | false (boolean) whether the block was accepted into the chain\n"
            "\nExamples:\n"
            + HelpExampleCli("submitauxblock", "\"hash\" \"auxpowhex\"")
            + HelpExampleRpc("submitauxblock", "\"hash\" \"auxpowhex\"")
        );

    const uint256 hash = ParseHashV(params[0], "hash");
    CAuxPow auxpow;
    if (!DecodeAuxPow(auxpow, params[1].get_str()))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "AuxPow decode failed");

    return SubmitAuxBlockHelper(hash, auxpow);
}
