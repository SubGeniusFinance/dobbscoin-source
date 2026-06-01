# AuxPoW regtest smoke tests

Self-contained shell tests for the v0.12.0 AuxPoW RPC contract. Run against
a freshly built `src/dobbscoind` to verify the daemon-side of merge-mining
is wired up before standing up real MiningCore.

## What's tested

`regtest-smoke.sh` — exercises createauxblock / submitauxblock end-to-end:

| Test | What it confirms |
|---|---|
| 1 | createauxblock refuses pre-activation (regtest fork @ block 200) |
| 2 | Post-activation response has 7 documented fields, chainid == 176 |
| 3 | submitauxblock rejects unknown block hash |
| 4 | submitauxblock rejects malformed auxpow hex |
| 5 | Repeat createauxblock returns cached template (or fresh if mempool drifted) |
| 6 | Advancing the chain tip invalidates the cache |

What it does **NOT** test: actually constructing a CAuxPow that satisfies
the parent-PoW check. That requires scrypt mining of a synthetic parent
block, which is what real MiningCore does. Full end-to-end with real
parent PoW is a runbook (below), not a unit test.

## Running

```bash
make -j$(nproc) -C src dobbscoind dobbscoin-cli
./qa/auxpow-smoke/regtest-smoke.sh
```

Defaults assume `src/dobbscoind` and `src/dobbscoin-cli` exist relative to
cwd-as-`qa/auxpow-smoke/`. Override:

```bash
./regtest-smoke.sh /path/to/dobbscoind /path/to/dobbscoin-cli
```

The script provisions a temp datadir, starts the daemon with
`-rpcuser=auxpow -rpcpassword=test`, mines 250 blocks past activation,
runs the tests, and cleans up on exit.

## Real-MiningCore runbook

Once the smoke tests pass, point real MiningCore at the same regtest
daemon to verify the full round-trip including parent-PoW:

1. Run `dobbscoind -regtest -rpcuser=… -rpcpassword=…` with `-rpcport`
   set to whatever your MiningCore config expects.
2. Generate enough regtest blocks to cross activation (block 200).
3. In MiningCore's coin configuration, set the merge-mining child pool
   to use:
   - `createAuxBlock` RPC: `createauxblock`
   - `submitAuxBlock` RPC: `submitauxblock`
   - `chainId`: `176` (0x00B0)
   - Coin algorithm: `scrypt`
4. Start MiningCore. Point an actual scrypt miner (cgminer, ccminer, etc.)
   at MiningCore's stratum endpoint.
5. Observe submitauxblock landing — `getblockchaininfo` should show the
   chain advancing with VERSION_AUXPOW bit set in block versions.

If a real MiningCore deployment confirms blocks getting accepted, the
v0.12.0 RPC contract is ready for mainnet activation at block 2,000,000.
