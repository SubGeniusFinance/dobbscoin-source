========================================
Dobbscoin Core integration/staging tree
========================================
Dobbscoin (BOB) is the Official CryptoCurrency of
The Church of the SubGenius. http://www.subgenius.com
The ONLY crypto-currency accepted on the Pleasure Saucers.
Backed By Nothing, & Powered By Everything!!

Dobbscoin (BOB) Homepage: https://www.dobbscoin.info

•  Maybe you've come into a large inheritance, or your income just suddenly popped. 
•  Maybe you gave birth to quintuplets or been recently divorced. Or maybe you just feel uneasy about your money; where it's going or how far it will take you in the future.

Whatever your problem is, Dobbscoin can help you.

Will you join The Conspiracy's mindless atheistic unknowing servitude to the Elder Bankers of the Universe and their MINIONS in some hideous World Government, ..or will you GET SLACK and FIGHT FOR FREEDOM as a zeal-crazed Priest-Warrior for ODIN?

•  SubGenius Finance : https://SubGenius.Finance 
Where Sub-Culture becomes Capital

Eternal Salvation or Triple your Money Back!!

------------------
What is Dobbscoin?
------------------
Dobbscoin is an excremental digital currency that enables the sending of
instant Slack to anyone anywhere. Dobbscoin uses peer-to-peer technology
to operate with no central authority: managing transactions and issuing
Slack is carried out collectively by the network.

For more information, as well as an immediately useable binary version of
the Dobbscoin Core software, see:
https://github.com/SubGeniusFinance/dobbscoin-source/releases


Building Dobbscoin on Linux
---------------------------
Dobbscoin can be built on modern Linux distributions such as Ubuntu 22/24
and Debian using standard system libraries.

Most users can build Dobbscoin with the following commands:

    git clone https://github.com/SubGeniusFinance/dobbscoin-source.git
    cd dobbscoin-source

    ./autogen.sh
    ./configure
    make

After building, verify the version:

    ./src/dobbscoind --version

NOTE:
Compatible Wallet Builds (Berkeley DB 4.8)
------------------------------------------
You may see the following message when building Dobbscoin:

    Found Berkeley DB other than 4.8, required for portable wallets

This is not an error and not a bug in Dobbscoin.

It is a long-standing compatibility requirement shared across many
Bitcoin-family cryptocurrencies, including historical versions of
Bitcoin itself. Modern versions of Bitcoin Core do not use Berkeley
DB at all for the wallet. That change happened in Bitcoin Core
0.17–0.21 era, when the wallet backend was redesigned and later
migrated to SQLite by default.

So today:
Bitcoin Core 25.x / 26.x / 27.x
Wallet storage: SQLite
Berkeley DB: legacy only

Why does this exist?
--------------------
Early versions of Bitcoin standardized on Berkeley DB version 4.8
for wallet storage. Over time, newer versions of Berkeley DB changed
internal behavior. To ensure that wallet files remain portable and
compatible across different systems and builds, many projects continue
to use Berkeley DB 4.8 for production and release builds.

Because modern operating systems ship newer Berkeley DB versions,
developers often install a local compatible copy specifically for
building wallet software.

This is normal across the cryptocurrency ecosystem.

Do I need Berkeley DB 4.8?
--------------------------
Most users do NOT need to worry about this.
Use the standard build unless you are:

- Building official release binaries
- Running production infrastructure
- Operating an exchange or custodial service
- Maintaining long-term wallet compatibility
- Learning how cryptocurrency build systems work

If you are simply running a node, experimenting, or developing,
the default build is usually sufficient.

---------------------------------------------------------------
Installing the Compatible Library (Recommended for Maintainers)
---------------------------------------------------------------
Dobbscoin provides a helper script that automatically installs the
required Berkeley DB 4.8 libraries safely into your home directory.

This does NOT modify your operating system or replace system libraries.

Run:

    ./contrib/install-db4.sh

The script will:

- Download Berkeley DB 4.8 source code
- Build the required libraries
- Install them locally into:

        $HOME/db4

No administrative privileges are required.

----------------------------------------------
Building Dobbscoin with Compatible Berkeley DB
----------------------------------------------

After installing the compatible library, build Dobbscoin using:

    ./autogen.sh
    ./configure --with-bdb=$HOME/db4
    make

---------------------------
Standard Build (Most Users)
---------------------------
If you do not need strict wallet portability, you can simply build
Dobbscoin using your system libraries:

    ./autogen.sh
    ./configure
    make


Summary
-------
Seeing a Berkeley DB 4.8 message is normal in the cryptocurrency world.

Dobbscoin includes an installer script to make the process simple,
safe, and reproducible.

Nothing is broken — this is just part of how many cryptocurrency
wallet systems maintain compatibility over time.

License
-------
Dobbscoin Core is released under the terms of the MIT license.
See COPYING for more information or see:
http://opensource.org/licenses/MIT.


Development process
-------------------
Developers work in their own trees, then submit pull requests when they think
their feature or bug fix is ready. If it is a simple/trivial/non-controversial
change, then one of the Dobbscoin development team members simply pulls it.
If it is a more complicated or potentially controversial change, then the patch
submitter will be asked to start a discussion (if they haven't already) on the
FORUMS:

https://subgenius.finance/smf/


The patch will be accepted if there is broad consensus that it is a good thing.
Developers should expect to rework and resubmit patches if the code doesn't
match the project's coding conventions (see doc/coding.md) or are controversial.


The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. Tags are created regularly to indicate new official,
stable release versions of Dobbscoin.


Testing
-------
Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and
help out by testing other people's pull requests, and remember this is a
security-critical project where any mistake might cost people lots of time.


Automated Testing
-----------------
Developers are strongly encouraged to write unit tests for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled in configure) with:

    make check

Every pull request is built for both Windows and Linux on a dedicated server,
and unit and sanity tests are automatically run.

See:

https://github.com/TheBlueMatt/test-scripts
for the build/test scripts.


Manual Quality Assurance (QA) Testing
-------------------------------------
Large changes should have a test plan, and should be tested by somebody other
than the developer who wrote the code.
