# Dobbscoin Core — (BOB)

[![Release](https://img.shields.io/github/v/release/SubGeniusFinance/dobbscoin-source?label=release)](https://github.com/SubGeniusFinance/dobbscoin-source/releases)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](COPYING)
[![CI](https://github.com/SubGeniusFinance/dobbscoin-source/actions/workflows/release.yml/badge.svg)](https://github.com/SubGeniusFinance/dobbscoin-source/actions/workflows/release.yml)

> # **MONETIZED SLACK.**
> ### Backed By Nothing. Powered By Everything.
>
> **(BOB)** is the Official CryptoCurrency of **The Church of the SubGenius** — the ONLY currency accepted on the Pleasure Saucers, and the only ledger "Bob" Himself would dignify by salesmanship.
>
> *Eternal Salvation or Triple Your Money Back.*

**<https://dobbscoin.info>** · **<https://subgenius.finance>** — *where Sub-Culture becomes Capital.*

Maybe your income just suddenly popped. Maybe you've inherited a fortune, birthed quintuplets, been recently divorced, or you simply woke up in a cold sweat at 3am knowing — *knowing* — that the Elder Bankers of the Universe are slowly draining your essential fluids through the bank app on your phone.

Whatever your problem is, **(BOB) can help you.**

The Conspiracy wants you mediocre, taxable, and on autopay. (BOB) wants you free. **Choose.**

---

## ⚠ Upgrade notice — two hard forks incoming

| Activation | Block | ETA | Old node fate |
|---|---|---|---|
| **LWMA-3 + emergency-diff + 100-block finality** | **1,888,808 → 1,888,888** | ~2026-07-16 | Forked off |
| **AuxPoW merge-mining** (chain ID `0x00B0`) | **2,000,000** | ~2026-12-17 | Forked off |

**Run v0.13.0.** Anything older gets left behind on a dead chain. The Pinkboys can keep it.

---

## Contents
- [What is (BOB)?](#what-is-bob)
- [Quick start](#quick-start)
- [Building from source](#building-from-source-linux)
- [Wallet builds — Berkeley DB 4.8](#wallet-builds--berkeley-db-48)
- [Network parameters](#network-parameters)
- [Ecosystem](#ecosystem)
- [Contributing](#contributing)
- [Testing](#testing)
- [License](#license)

---

## What is (BOB)?

(BOB) is an excremental digital currency that sends **instant Slack** to anyone, anywhere. No central authority. No board of directors. No quarterly earnings call. Just a peer-to-peer network of weirdos, mystics, miners, and tipbot enjoyers issuing Slack collectively, on a 2-minute heartbeat, since January 2014.

It is **scrypt** under the hood, **flat 1.5 (BOB) per block forever**, and **NOT for Sale**. You earn it, mine it, or get tipped it. You do not buy it. *(NOT FINANCIAL ADVISORS. NOT FINANCIAL ADVICE.)*

Pre-built binaries for Linux (daemon, Qt5, AppImage) and Windows 64-bit live on the releases page:

→ **<https://github.com/SubGeniusFinance/dobbscoin-source/releases>**

---

## Quick start

```bash
git clone https://github.com/SubGeniusFinance/dobbscoin-source.git
cd dobbscoin-source
./contrib/install-db4.sh                       # builds BDB 4.8 into $HOME/db4 (no root)
./autogen.sh
./configure --with-bdb=$HOME/db4
make -j$(nproc)
./src/dobbscoind --version                     # should say 0.13.0
```

That's it. You now have a working (BOB) node and wallet. Praise "Bob".

---

## Building from source (Linux)

Tested on Ubuntu 22.04 / 24.04 and recent Debian. Install build deps:

```bash
sudo apt install build-essential libssl-dev libboost-all-dev libevent-dev \
                 libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev-tools \
                 libprotobuf-dev protobuf-compiler libqrencode-dev
```

Then run the [Quick start](#quick-start) block above.

For Windows, macOS, and cross-compile, see [`doc/build-*.md`](doc/).

---

## Wallet builds — Berkeley DB 4.8

If `configure` says:

> `Found Berkeley DB other than 4.8, required for portable wallets`

…**this is not a bug.** (BOB) inherits the Bitcoin Core 0.10 wallet format, which requires **Berkeley DB 4.8.30** for `wallet.dat` portability. Modern distros ship 5.x / 6.x, which cannot open the legacy format. Every serious Bitcoin-family fork still does this.

The helper script installs a local copy into `$HOME/db4` — no root, no system-library tampering:

```bash
./contrib/install-db4.sh
./configure --with-bdb=$HOME/db4
make -j$(nproc)
```

Skip BDB only if you are running a **node-only** install with no wallet. If you need to hold or move (BOB), you need 4.8. End of debate.

---

## Network parameters

| Parameter | Value |
|---|---|
| Proof of work | **scrypt** |
| Block target | **2 minutes** (since block 68425) |
| Difficulty algorithm | DigiShield V4 → **LWMA-3** at block 1,888,808 → **emergency-diff** at 1,888,888 |
| Max reorg depth | none → **100 blocks** at activation |
| Merge mining | **AuxPoW**, chain ID **`0x00B0`**, at block 2,000,000 |
| Subsidy | **1.5 (BOB) per block, forever** (since block 951753) |
| Halvings | Capped at block 951752 — chain is mildly inflationary by design |
| Tx version | v1 only |
| Genesis | January 2014 (Bitcoin Core 0.10-era fork) |

> There is **no 21M supply cap.** That was 2014 marketing. The `MAX_MONEY` sanity ceiling sits at 10B (BOB) — at the current emission rate, ~25,000 years away. The chain is intentionally, gently inflationary. **Slack is not scarce.**

---

## Ecosystem

| | |
|---|---|
| **Block explorer** | <https://explorer.dobbscoin.info> |
| **Mining pool** | <https://pool.dobbscoin.info> |
| **Faucet** | <https://faucet.dobbscoin.info> |
| **Rich list** | <https://explorer.dobbscoin.info/richlist> |
| **"Bob" Bank** — custodial web wallet | <https://subgenius.finance/bobbank> |
| **Android APK** | <https://subgenius.vip/dobbscoin.apk> |
| **Bridge → wBOB on Gnosis** | <https://bridge.subgenius.finance> |
| **wBOB trading** (primary, Oku) | [oku.trade — wBOB pair](https://oku.trade/swap?inputChain=gnosis&inToken=0x13550ae65f22A36f60A50d625B70b58666488263&outToken=0xe91d153e0b41518a2ce8dd3d7944fa863463a97d) |
| **Forum** | <https://subgenius.finance/smf/> |

> **(BOB) is NOT for Sale.** Never has been, never will be. You acquire it, you earn it, you mine it, you get tipped it. The Conspiracy can keep its order book.

---

## Contributing

This is an open project. Patches welcome.

- **Trivial / uncontroversial** — open a PR, it gets pulled.
- **Anything that touches consensus, the wallet, or user-facing behavior** — open a thread on the [forum](https://subgenius.finance/smf/) first. Hard forks are not surprise parties.
- Match the style in [`doc/coding.md`](doc/coding.md). Don't reformat the world.

`master` is built and tested but not guaranteed stable. **Tags are the stable line** — currently `v0.13.0`.

---

## Testing

Build and run the unit tests:

```bash
make check
```

Every PR is built via GitHub Actions — see [`.github/workflows/release.yml`](.github/workflows/release.yml).

Large changes need a test plan and need to be tested by **someone other than the author.** "It works on my machine" is how chains die.

---

## License

MIT. See [`COPYING`](COPYING) or <https://opensource.org/licenses/MIT>.

---

<p align="center"><em><strong>Praise "Bob". GET SLACK. The Devival is forever.</strong></em></p>
