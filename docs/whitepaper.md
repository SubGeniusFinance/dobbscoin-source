# Dobbscoin: A Peer-to-Peer Slack System

**The Official CryptoCurrency of the Church of the SubGenius — Backed by Nothing, Powered by Everything**

*v1.0 — Submitted at the convergence of `0.10.808.999`, after twelve years of stubborn existence.*

---

## Abstract

Dobbscoin (ticker: **(BOB)**) is a peer-to-peer cryptocurrency forked from
Bitcoin Core 0.10.x, in operation continuously since **block height zero
on January 7, 2014**. Its sole purpose is the transfer of **Slack** —
the One True Currency of the Church of the SubGenius — between sentient
beings without the involvement of any priest, banker, central authority,
or other agent of the Conspiracy. There is no foundation. There are no
investors. There is no premine. There is no marketing budget. There is
J. R. "Bob" Dobbs, smoking His pipe in the marketplace of ideas, and
there is the chain. That has been enough for twelve years and counting.

This document describes Dobbscoin's chain parameters, its modernization
trajectory, its emerging cross-chain expansion via the wBOB bridge, and
the theological premises that justify any of it.

---

## 1. The Problem

The world's monetary systems are operated by the Conspiracy — a
loose-knit confederacy of bankers, regulators, bureaucrats, brand
managers, and reply-guys whose collective ambition is to convert all
human activity into a debt-bearing instrument denominated in their
preferred unit of account. The result is a financial substrate
optimized for one outcome: the steady transfer of Slack from those
who produce it to those who already have too much of it.

Bitcoin proposed an exit. It worked, more or less. But Bitcoin's
community, in an act of unforced theological capitulation, reorganized
itself around the worship of "store of value," "digital gold," and
"corporate treasury allocation" — the Conspiracy's exact vocabulary,
applied to a slightly different rail. Whatever Bitcoin was meant to be,
it is now a hedge fund product with a community Slack channel.

Dobbscoin takes a different position. It is a coin for people who think
the existing financial system is *funny*, who believe the appropriate
response to monetary tyranny is **mockery**, and who hold that the only
honest valuation methodology is "well, somebody is willing to send some
of it to me, so it must be worth something." It is a sincerely-engineered
piece of unserious software.

This is what Bitcoin was supposed to feel like, before it grew up.

---

## 2. Genesis

The Dobbscoin chain began with the following coinbase message, embedded
in the scriptSig of the genesis transaction:

> `NY Times 05/Oct/2011 Steve Jobs, Apple's Visionary, Dies at 56`

This is not a financial-news headline in the Bitcoin tradition (which
referenced a UK bank bailout). It is a memento mori: a reminder that
even the most charismatic prophets of design and monetization
eventually return to dust, while the chain continues to extend itself
one block at a time. Bob Dobbs, by contrast, **cannot die**, because He
was never alive in the conventional sense. He simply *is*. The chain
agrees.

Genesis block details:

| Parameter | Value |
|---|---|
| `nTime` | 1389086657 (Tue Jan 7, 2014 06:04:17 UTC) |
| `nBits` | `0x1e0ffff0` |
| `nNonce` | `2084524493` |
| `nVersion` | 1 |
| Initial output | 1 satoshi (purely commemorative; unspendable) |
| Coinbase scriptSig | The Steve Jobs obituary above |

---

## 3. Chain Parameters

Dobbscoin inherits the bulk of Bitcoin Core 0.10.x's consensus, with
network-identifying constants chosen to make accidental cross-network
interaction impossible.

### 3.1 Mainnet

| Parameter | Value | Source |
|---|---|---|
| Network magic (`pchMessageStart`) | `a0 fb 17 83` | `src/chainparams.cpp:111-114` |
| Default P2P port | **19985** | `src/chainparams.cpp:116` |
| Default RPC port | 19984 | — |
| DNS seeds | `seed.dobbscoin.info`, `earlz.net` | `src/chainparams.cpp:156-157` |
| Address prefix (P2PKH) | `0x00` → `1`-addresses (visually identical to BTC; do not cross the streams) | `src/chainparams.cpp:159` |
| Address prefix (P2SH) | `0x05` → `3`-addresses | `src/chainparams.cpp:160` |
| BIP32 ext-pub | `0x0488B21E` (xpub) | `src/chainparams.cpp:162` |
| BIP32 ext-priv | `0x0488ADE4` (xprv) | `src/chainparams.cpp:163` |
| Decimals | 8 (satoshis) | upstream |
| **Proof-of-work hash** | **scrypt** | `src/primitives/block.cpp:19` (`HashScrypt(...)`) |
| **Tx version policy** | **v1 only — v2+ rejected at mempool** | `src/main.cpp:643`, `src/primitives/transaction.h:178` |
| **Target block time** | **2 minutes** (since block 68425) | `src/pow.cpp:477-478, 573` |
| **Difficulty retarget** | **KGW + DigiShield hybrid, every block** | `src/pow.cpp:561-579` (see §3.4) |
| **Block subsidy** | **1.5 (BOB) per block, perpetual** since block 951753 | `src/main.cpp:1310-1316` (see §4) |
| PoW difficulty floor | `~uint256(0) >> 20` (intentionally lower than Bitcoin's) | `src/chainparams.cpp:117` |
| Money sanity ceiling | `MAX_MONEY = 10,000,000,000 (BOB)` (validation cap, **not** a supply cap) | `src/amount.h:28` |
| Hard supply cap | **None** — see §4 | — |

The lower difficulty floor is intentional. Dobbscoin is not optimized
for the world's largest mining cartel; it is optimized for a hobbyist
with a CPU and a sense of humor to participate without being instantly
crowded out.

The transaction-version policy is the most consequential of the
"stayed-in-2014" choices: because version 2 transactions (BIP68 / CSV
relative timelocks) are rejected at the mempool, Dobbscoin does **not**
support sequence-based timelocks, payment channels with revocable
states, or native Lightning. This is not a planned feature gap — it is
a consequence of consensus rules that were never updated, and updating
them is not on the roadmap. (BOB) is a settlement-only asset.

### 3.4 Difficulty Retarget — KGW + DigiShield Hybrid

Dobbscoin's difficulty algorithm has switched modes three times by
height-gated `DiffMode` selection in `src/pow.cpp:561-579`:

| Block range | Mode | Algorithm |
|---|---|---|
| 0 – 13 578 | V1 | Bitcoin-style retarget every 2 016 blocks |
| 13 579 – 31 596 | V2 | **Kimoto Gravity Well** (labeled "BOB's Wormh0le (KGW)" in `src/pow.cpp:298`) |
| 31 597 – 68 424 | V3 | **DigiShield** with 10-minute target, every-block retarget |
| 68 425 – present | V4 | **DigiShield** with **2-minute target**, every-block retarget |

The V4 retarget (`src/pow.cpp:443-470, 532-558`) uses DigiShield's
amplitude-filter / asymmetric-bounds design: the next target is computed
as `target + (actual − target) / 8`, then clamped so difficulty can drop
faster than it can rise (the actual-timespan upper bound is `1.5 ×
target`, the lower bound is `0.75 × target`). The practical effect is
that a multi-pool driving 10× the network hashrate at the chain for an
hour will see difficulty climb out of their range within a few blocks,
and difficulty will drop back to honest-miner range within a few blocks
of their leaving. The drive-by-hashing exploit that hollowed out the
2013-2014 altcoin cohort no longer functions against (BOB).

### 3.2 Testnet

A standard Bitcoin-derived testnet runs alongside mainnet on port 39985,
intended for tooling development and bridge / wallet integration work.

### 3.3 Regtest

Used for deterministic local testing — useful in particular for
developing and verifying the wBOB bridge's Dobbscoin-side logic against
controlled chain conditions.

---

## 4. Emission Schedule

Dobbscoin's emission schedule is **not** the standard Bitcoin curve.
It started Bitcoin-shaped, was reshaped by a 2014 hard fork that cut
the block reward and the block time together, and was capped in 2017
to settle into a perpetual flat-subsidy regime. The full schedule, as
implemented in `src/main.cpp:1280-1319`:

```
Era            Block range            Subsidy        Cause / Source
─────────────────────────────────────────────────────────────────────────
Era 1 (10-min) 0       – 68 424      50 (BOB)/block   genesis params
                                                      (chainparams.cpp:125)
Adjustment     68 425  – 69 790      10 (BOB)/block   2-min hardfork — keep
                                                      total rate consistent
                                                      (main.cpp:1293-1295)
Era 2 (2-min)  69 791  – 951 752     starts at 10,    Bitcoin-style halving
                                     halves every     every 210 000 blocks
                                     210k blocks      (main.cpp:1297-1306)
Perpetual      951 753 – ∞           1.5 (BOB)/block  hard-coded floor
                                     forever          (main.cpp:1310-1312)
```

Concretely, the per-block subsidy followed this trajectory:

| Block height | Subsidy |
|---|---|
| 0 – 68 424 | 50 (BOB) |
| 68 425 – 69 790 | 10 (BOB) (transitional one-off) |
| 69 791 – 279 790 | 10 (BOB) |
| 279 791 – 489 790 | 5 (BOB) |
| 489 791 – 699 790 | 2.5 (BOB) |
| 699 791 – 909 790 | 1.25 (BOB) → floored to 1.5 by `nBlockRewardMinimumCoin` guard at `main.cpp:1316` |
| 909 791 – 951 752 | 1.5 (BOB) (floor active) |
| **951 753 – ∞** | **1.5 (BOB), hard-coded forever** |

(BOB) is therefore **not a deflationary asset**. After block 951 753 —
which the chain crossed in 2017 — the supply grows linearly at 1.5
(BOB) per ~2-minute block, or roughly **394 200 (BOB) per year**, in
perpetuity. There is no asymptote, no halvening event in the future,
no terminal supply.

The intent of the perpetual floor is to ensure that miners always
receive a non-trivial reward for securing the chain, even after
transaction fees have evolved (in either direction). A coin whose
miner subsidy goes to zero is a coin whose security model rests on
fee markets that may or may not materialize. (BOB)'s floor sidesteps
that question.

The `MAX_MONEY` constant defined in `src/amount.h:28` —
`10,000,000,000 (BOB)` — is a validation-time sanity ceiling (the
maximum value any single transaction output may carry), **not** a
chain supply cap. At the perpetual emission rate, the chain would
require approximately 25 000 years of uninterrupted mining to even
brush this number.

There was **no premine**. There was **no presale**. There were **no
"strategic partners"**. The genesis block produced one (1) satoshi as
a ceremonial output, and that satoshi is permanently unspendable due
to the historical Bitcoin Core peculiarity that the genesis coinbase
output's UTXO is not added to the database. The first spendable (BOB)
came from the first regular mining reward, available to anyone with a
working `dobbscoind`.

This is the SubGenius position on monetary issuance: equality of
opportunity to grind, not equality of allocation by committee. (BOB)
will continue to be issued at the perpetual rate for as long as
"Bob" continues to want it, and presumably therefore forever.

---

## 5. The Modernization Effort

Dobbscoin is, by any measure, *old code*. The reference daemon
(`dobbscoind`) descends from Bitcoin Core 0.10.x, which dates from
2015. Berkeley DB 4.8 — the wallet-format dependency required to read
historical wallet files — has been deprecated for years and does not
build cleanly on modern toolchains without intervention. The recent
modernization effort (visible in the repository's commit history at
versions `0.10.5` through `0.10.808.999`) has focused on:

1. **Build stabilization**: Deterministic Linux builds on Ubuntu 22 / 24
   and Debian, including a documented BDB-4.8 installer that patches
   the dependency for compatibility with modern GCC's stricter atomic
   semantics.
2. **Windows binary reproducibility**: Cross-build via the `depends/`
   tree, with a stabilized qrencode-3.4.3 toolchain integration.
3. **Backward compatibility above all**: No protocol changes. No fork.
   The wire format, consensus rules, and on-disk storage formats remain
   bit-for-bit compatible with every prior Dobbscoin node ever shipped.
   A 2015-era `dobbscoind` and a 2026-era `dobbscoind` see the same
   chain and validate identically.

The principle is simple: a coin's value is in part a function of how
hard it is to break, and the easiest way to break a coin is to "improve"
its consensus rules. Dobbscoin's consensus rules will be improved
exactly when "Bob" demands it, and not before. He has not yet demanded.

The modernization is purely tooling, packaging, and operational
ergonomics — making it easier for new operators to spin up nodes,
miners, and wallets without having to chase down arcane build issues.
The chain itself remains exactly as eccentric as the day it launched.

---

## 6. Cross-Chain Expansion: The wBOB Bridge

In 2026 Dobbscoin gained an EVM presence via the **wBOB bridge** — a
1:1 wrapped representation of (BOB) on **Gnosis Chain** (chainId 100).
This is detailed in a separate companion paper (`wbob-bridge` repo,
`docs/whitepaper.md`); the summary follows.

The bridge consists of:

- A **lock-mint** path: Users send (BOB) to a watched Dobbscoin address.
  After 6 confirmations, a 3-of-5 threshold of independent watcher
  EOAs sign an EIP-712 `MintAuthorization`, and any party can submit
  the assembled signatures to mint the equivalent wBOB on Gnosis.
- A **burn-release** path: Users call `requestWithdrawal()` on the
  Gnosis-side `BridgeController`, which burns their wBOB and emits an
  event. The bridge backend constructs and broadcasts a Dobbscoin
  payout transaction to the user-specified (BOB) address. Three Dobbscoin
  confirmations later, the order is `COMPLETED`.

The bridge does **not** alter Dobbscoin's consensus, supply, or wire
protocol. It is an external system that observes the canonical chain,
locks (BOB) into addresses that no single party can spend alone, and mints
a corresponding token on a chain that has actual DeFi rails.

This means (BOB) holders may now:

- Provide liquidity in Gnosis-native DEXes (current liquidity is on
  Oku, with execution available via CoW Swap at
  `https://swap.cow.fi/#/100/swap/xDAI/wBOB`).
- Use (BOB) as collateral in Gnosis-native lending markets (when one
  emerges that recognizes the asset).
- Participate in xDAI-denominated yield strategies without selling (BOB).

The bridge's existence is a force multiplier on Slack: (BOB) no longer has
to leave the SubGenius cosmology to participate in the broader DeFi
ecosystem. The Conspiracy's tools, used against the Conspiracy.

---

## 7. Tokenomics, in So Far As We Refuse to Have Any

Dobbscoin's economic model is its perpetual emission curve (see §4)
applied to a coin that does not pretend to be money. The chain itself
has:

- No development fund
- No consensus-layer treasury
- No quarterly token unlocks
- No "ecosystem grant" stash baked into the protocol
- No buybacks, burns, halvening-bonus tax-loss-harvesting yield-flywheel
  governance-token meta-narrative welded to consensus

There is one (1) thing: a chain that produces (BOB) at a known, schedule-
gated rate, for whichever miners care enough to point hash power at it.
That's it. That's the whole monetary policy.

Anyone proposing to "add tokenomics" to Dobbscoin's *consensus layer*
is, by definition, proposing to make Dobbscoin into a different coin.
We're not interested. The chain is the chain.

(There is, separately, a community-coordination DAO being chartered on
Gnosis Chain — the CoSG-DAO — that holds a community treasury, funds
ecosystem work, and is otherwise opinionated about Slack. That DAO
sits *alongside* (BOB), funded by donations and opt-in revenue streams,
and never touches the chain's consensus rules or supply schedule.
Details in the wBOB Bridge whitepaper, §7.)

"Bob" approves.

---

## 8. Governance, in So Far As We Refuse to Have Any

There is no on-chain governance. There is no signaling, voting,
delegation, or gauge-weighting. The chain's parameters are what they
are because they were that way at genesis, and any change to them
requires the entire network of nodes and miners to agree to run
software that implements the change. This is — and we cannot stress
this enough — *exactly how Bitcoin was supposed to work* before
governance theater was retrofitted onto the rest of the industry.

If a change is genuinely good, it will be obvious; node operators will
upgrade, miners will follow, and the chain will continue. If a change
is bad, the network will reject it through inaction, and the chain will
also continue. The governance mechanism is "do node operators run the
software," and the only legitimate way to lobby for a change is to
publish the diff and let people make up their own minds.

We do not anticipate changes.

---

## 9. Roadmap

Roadmaps for SubGenius-aligned projects are inherently suspicious — they
imply a level of advance planning incompatible with the spontaneous
generation of Slack — but in deference to ecosystem expectations:

- **Continuous**: Build stability across modern Linux distributions
  and Windows. Reproducible release artifacts.
- **2026 H2**: Widen wBOB bridge audit and redundancy. Onboard
  independent watcher operators. Verified contracts on Gnosisscan.
- **2026 H2**: Submission of (BOB) and wBOB to canonical token registries
  (Gnosis tokenlists, CoinGecko, CoinMarketCap).
- **2027+**: Whatever "Bob" tells us to do. Likely nothing dramatic.
  The chain is the chain.
- **X-Day** (continuously deferred to next year, as has been the
  Church's tradition since 1980): Total Slack. Achievable in principle;
  unachievable in practice. The ledger continues either way.

We commit to **no protocol changes** as a feature, not a bug.

---

## 10. Conclusion

Dobbscoin is twelve years of a chain refusing to die quietly. It has
no marketing department. It has no influencer endorsements. It has
no exchange listings to speak of (and the bridge is, deliberately, an
end-run around the need for them). What it has is uptime, an emission
schedule that hasn't deviated from spec, a community whose membership
test is having a working sense of humor, and a ticker symbol that
spells "Bob."

This is sufficient. The Conspiracy will continue to call it a joke.
The joke will continue to clear blocks every two minutes.

**PRAISE "Bob".**

---

## Appendix A: Network Constants Reference

For implementers building wallets, explorers, miners, and bridge
components — these are the load-bearing magic numbers.

```
Mainnet:
  pchMessageStart = a0 fb 17 83
  P2P port        = 19985
  PUBKEY prefix   = 0x00  ("1" addresses)
  SCRIPT prefix   = 0x05  ("3" addresses)
  SECRET prefix   = 0x80
  xpub version    = 0x0488B21E
  xpriv version   = 0x0488ADE4
  Genesis hash    = (see HASHGENESISBLOCK in src/chainparamsmainnet.h)
  Genesis nTime   = 1389086657
  Genesis nNonce  = 2084524493
  Genesis nBits   = 0x1e0ffff0

Testnet:
  pchMessageStart = fa da 0b bf
  P2P port        = 39985

Regtest:
  pchMessageStart = fa bf b5 da
  P2P port        = 18444 (legacy default)
```

## Appendix B: Build & Operation

The canonical reference implementation is the `dobbscoind` daemon and
`dobbscoin-qt` graphical wallet, in this repository at
`github.com/SubGeniusFinance/dobbscoin-source`. Build instructions for modern
Linux are documented in `docs/build-linux.md`. Windows reproducible
builds use the `depends/` cross-compile tree.

Run a node:
```sh
./autogen.sh && ./configure && make
src/dobbscoind -daemon
src/dobbscoin-cli getblockchaininfo
```

Wallet files are encrypted on disk (Berkeley DB 4.8 format,
historically) and may be encrypted with a passphrase. Do not lose
your passphrase. "Bob" does not retrieve forgotten passphrases.

## Appendix C: Resources

| Resource | URL |
|---|---|
| Reference daemon source | https://github.com/SubGeniusFinance/dobbscoin-source |
| wBOB bridge source | https://github.com/dobbscoin/wbob-bridge |
| wBOB bridge contracts | https://github.com/dobbscoin/wbob-contracts |
| wBOB bridge UI | https://bridge.subgenius.finance |
| Project homepage | https://dobbscoin.info |
| SubGenius mothership | https://www.subgenius.com |

---

*"Bob" knows what He's doing. The miners just hash the blocks.*
