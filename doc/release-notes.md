Dobbscoin Core version 0.10.808.999 is now available from:

  <https://github.com/SubGeniusFinance/dobbscoin-source/releases>

This is a Bad Ass last of the 0.10.xx series release, bringing minor bug 
fixes and all kinds of bitching updates. If you care about that stuff go
read the comit history on github.
 
It is recommended to upgrade to this version.

We'll jump ahead a bunch next and work toward 
Bitcoin Core 0.14.x (2017 era) 
— SegWit Era / Performance & DoS Hardening code base.


But here's where we're at right now.

Version 0.10.808.999
---------------------

Deterministic installation and deployment milestone.

Changes included:

- Introduction of deterministic Ubuntu installer
- Automated dependency installation workflow
- Local Berkeley DB 4.8 installation support
- Optional Qt GUI build selection
- Local or system-wide installation options
- Binary verification during installation
- Safe default deployment behavior
- Reduced binary size through standard stripping
- Automatic creation of expected data directory structure
- Improved operator guidance during installation
- Consistent cross-machine deployment validation
- Clean installation behavior on fresh systems

This version establishes the first fully reproducible installation workflow for Dobbscoin.

It represents the completion of the foundational modernization phase.

---

Operational Impact
------------------

This release improves reliability and maintainability without altering network behavior.

Operators can expect:

- Predictable installation results
- Reduced deployment friction
- Stable runtime behavior
- Improved portability across systems
- Clear upgrade path for future versions

No operational changes are required.

Please report bugs using the issue tracker at github:

  <https://github.com/SubGeniusFinance/dobbscoin-source/issues>


Dobbscoin Core Release Notes
============================

Version 0.10.808.999
---------------------

This release represents the culmination of the Dobbscoin 0.10 modernization and stabilization cycle.  
The primary objective of this work was to ensure that the Dobbscoin 0.10 codebase builds reliably on modern Linux systems while preserving full compatibility with the existing blockchain and network.

No consensus, protocol, or wallet format changes were introduced during this cycle.

This version establishes a reproducible and maintainable baseline suitable for production deployment and future staged upgrades toward later Bitcoin Core architectural eras.

Key Characteristics
-------------------

- Deterministic build and installation workflow
- Reliable compilation on modern Ubuntu systems (22.04 and 24.04)
- Local Berkeley DB 4.8 dependency isolation
- Optional Qt GUI build support
- Predictable installation behavior for both user-level and system-wide deployments
- Improved operational safety and deployment clarity
- Stable foundation for future modernization work

Compatibility
-------------

This release maintains full compatibility with:

- Existing Dobbscoin blockchain data
- Existing wallet.dat files
- Existing network peers
- Existing mining infrastructure

No reindex or migration is required.

---

Summary of Work by Version Range
================================

Version 0.10.3 – 0.10.4
------------------------

Early stabilization and environment alignment work.

Changes included:

- Initial cleanup of legacy build assumptions
- Modern compiler compatibility adjustments
- Toolchain validation on contemporary Linux distributions
- Dependency discovery improvements
- Removal of obsolete build expectations
- Early documentation alignment

These changes focused on restoring reliable builds after long-term toolchain drift.

No runtime behavior changes were introduced.

---

Version 0.10.5
---------------

Establishment of a stable operational baseline.

Changes included:

- Successful end-to-end compilation on modern Linux systems
- Verification of daemon, CLI, and Qt wallet functionality
- Correction of compiler compatibility issues
- Stabilization of runtime initialization behavior
- Standardization of repository layout and build workflow
- Validation of network operation under modern environments
- Initial reproducible build validation

This version became the first confirmed stable baseline for ongoing modernization work.

---

Version 0.10.6 – 0.10.807
--------------------------

Incremental maintenance and reliability improvements.

Changes included:

- Continued compiler compatibility adjustments
- Build system resilience improvements
- Dependency detection reliability improvements
- Minor code hygiene updates
- Warning reduction under modern toolchains
- Improved portability across Linux environments
- Internal documentation improvements
- Consistency fixes across build scripts

These versions represent iterative reliability work rather than functional feature changes.

No consensus or protocol behavior changes were introduced.

---

Version 0.10.808
-----------------

Modern build system stabilization.

Changes included:

- Reliable compilation on modern GCC toolchains
- Resolution of legacy Berkeley DB detection behavior
- Improved handling of modern dependency layouts
- Consistent build behavior across clean systems
- Strengthened portability guarantees
- Finalization of stable build configuration patterns

This version marked the transition from manual build workflows to repeatable installation procedures.


Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dobbscoin-Qt (on Mac) or
dobbscoind/dobbscoin-qt (on Linux).

Thank you to every builder who ever touched Dobbscoin (BOB) -whether you knew
it or not. Super Huge thanks to X_Slacks, earlz, EndCiv, BuZz, patrykw, cyotee,
Jim_Gilmore, matt_dillon, and everyone who idled in strange channels making
this project the lighthouse for educational talent and treasure trove of
FREE SLACK that it's been to all who participate in any capacity: be it wallet
node, graphic design, web development, social media administration or chat pal. 


Downgrade warning
------------------
Because release 0.10.0 and later makes use of headers-first synchronization and
parallel block download (see further), the block files and databases are not
backwards-compatible with pre-0.10 versions of Dobbscoin Core or other software:

* Blocks will be stored on disk out of order (in the order they are
received, really), which makes it incompatible with some tools or
other programs. Reindexing using earlier versions will also not work
anymore as a result of this.

* The block index database will now hold headers for which no block is
stored on disk, which earlier versions won't support.

If you want to be able to downgrade smoothly, make a backup of your entire data
directory. Without this your node will need start syncing (or importing from
bootstrap.dat) anew afterwards. It is possible that the data from a completely
synchronised 0.10 node may be usable in older versions as-is, but this is not
supported and may break as soon as the older version attempts to reindex.

This does not affect wallet forward or backward compatibility.

Notable changes
===============

This fixes a serious problem on Windows with data directories that have non-ASCII
characters (https://github.com/SubGeniusFinance/dobbscoin-source/issues/6078).

For other platforms there are no notable changes.

For the notable changes in 0.10, refer to the release notes
at https://github.com/SubGeniusFinance/dobbscoin-source/blob/v0.10.0/doc/release-notes.md


We Believe:
This stuff came from the BlueMat Shitcoin Generator

0.10.2 Change log
=================

Detailed release notes follow. This overview includes changes that affect external
behavior, not code moves, refactors or string updates.

Wallet:
- `824c011` fix boost::get usage with boost 1.58

Miscellaneous:
- `da65606` Avoid crash on start in TestBlockValidity with gen=1.
- `424ae66` don't imbue boost::filesystem::path with locale "C" on windows (fixes #6078)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Cory Fields
- Gregory Maxwell
- Jonas Schnelli
- Wladimir J. van der Laan

And all those who contributed additional code review and/or security research:

- dexX7
- Pieter Wuille
- vayvanne

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/dobbscoin/).
