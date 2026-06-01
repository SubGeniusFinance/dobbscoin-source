# Dogecoin AuxPoW reference

Files pulled from `dogecoin/dogecoin` for use as the reference implementation
of the AuxPoW port targeted for (BOB) v0.12.0.

**Source repo:** https://github.com/dogecoin/dogecoin
**Pinned commit:** `699f62ccba4e9c886a44d578c3923b4e14ef0a08` (master HEAD on 2026-06-01)

These files are **reference only** — do not compile into the (BOB) tree from
this directory. They are kept here so the port diff has something to point at.

## What each file is

| File | Upstream path | Role |
|---|---|---|
| `src/auxpow.h` | `src/auxpow.h` | The `CAuxPow` class — parent-block-coinbase commitment validator + merkle branch check |
| `src/auxpow.cpp` | `src/auxpow.cpp` | `CAuxPow::check()` and merkle-root extraction logic |
| `src/pureheader.h` | `src/primitives/pureheader.h` | The 80-byte "pure" header split out so AuxPoW can serialize as an optional trailer. Includes chain-ID encoding in `nVersion` (upper 16 bits) |
| `src/pureheader.cpp` | `src/primitives/pureheader.cpp` | Pure-header `GetHash()` / `GetPoWHash()` |
| `src/block.h` | `src/primitives/block.h` | `CBlockHeader` inheriting from `CPureBlockHeader`, with optional AuxPoW field |
| `src/block.cpp` | `src/primitives/block.cpp` | Block serialization with optional AuxPoW trailer |
| `src/rpc_auxpow.cpp` | `src/rpc/auxpow.cpp` | `createauxblock` / `submitauxblock` RPC handlers. MiningCore speaks this API |
| `test/auxpow_tests.cpp` | `src/test/auxpow_tests.cpp` | Boost.Test fixtures for the AuxPoW validator |

## Port surface against (BOB) `src/`

| Reference file | Lands in (BOB) tree as | Notes |
|---|---|---|
| `auxpow.{h,cpp}` | `src/auxpow.{h,cpp}` | New files. ~471 LOC, port mostly verbatim |
| `pureheader.{h,cpp}` | `src/primitives/pureheader.{h,cpp}` | New files. Refactor (BOB)'s `CBlockHeader` to inherit from this |
| `block.{h,cpp}` | edits to `src/primitives/block.{h,cpp}` | (BOB)'s `block.cpp:19` already uses `HashScrypt` — push that into pureheader |
| `rpc_auxpow.cpp` | `src/rpc/auxpow.cpp` (new) | Wire into `rpc/server.cpp` table. MiningCore wants `createauxblock`/`submitauxblock` |
| `auxpow_tests.cpp` | `src/test/auxpow_tests.cpp` | Port verbatim, swap chain-ID constant |

## Consensus parameters (locked 2026-06-01)

1. **(BOB) chain ID:** `0x00B0` ("B0b" mnemonic). Encoded in upper 16 bits of
   `nVersion`. Verify non-collision against the active merge-mined-on-LTC set
   before tagging v0.12.0.
   Reference list: https://en.bitcoin.it/wiki/Merged_mining_specification#Chain_ID
2. **Activation height:** mainnet block **2,000,000** (~2026-12-17).
   Sits ~111k blocks (~154 days) past LWMA-3 activation at 1,888,808.
   Constant: `HARDFORK_AUXPOW_MAIN = 2000000` in `src/pow.h`.
3. **Mode: additive.** Post-fork, both solo-mined headers (no AuxPoW trailer)
   and AuxPoW-extended headers are valid. The user remains able to solo-mine
   direct. `CheckProofOfWork` branches on `nVersion & VERSION_AUXPOW`.
4. **Pool integration:** MiningCore's AuxPoW support is via the
   `createauxblock`/`submitauxblock` RPC pair — same as Namecoin/Doge. Our RPC
   port must match those signatures exactly.

## What is NOT here

- `src/rpc/mining.cpp` — `getblocktemplate` changes for AuxPoW commitments are
  small and live alongside existing template code. Easier to diff against
  upstream Bitcoin Core mining.cpp when we get there.
- `src/chainparams.cpp` — Doge's chain-ID and activation-height constants. We
  set our own values; nothing to port.
- `src/validation.cpp` (or in 0.10-era, `src/main.cpp`) — `CheckProofOfWork`
  fork into the AuxPoW path. Small edit; not worth pre-staging.
