DIST_SUBDIRS = secp256k1
AM_LDFLAGS = $(PTHREAD_CFLAGS) $(LIBTOOL_LDFLAGS)


if EMBEDDED_LEVELDB
LEVELDB_CPPFLAGS += -I$(srcdir)/leveldb/include
LEVELDB_CPPFLAGS += -I$(srcdir)/leveldb/helpers/memenv
LIBLEVELDB += $(builddir)/leveldb/libleveldb.a
LIBMEMENV  += $(builddir)/leveldb/libmemenv.a

# NOTE: This dependency is not strictly necessary, but without it make may try to build both in parallel, which breaks the LevelDB build system in a race
$(LIBLEVELDB): $(LIBMEMENV)

$(LIBLEVELDB) $(LIBMEMENV):
	@echo "Building LevelDB ..." && $(MAKE) -C $(@D) $(@F) CXX="$(CXX)" \
	  CC="$(CC)" PLATFORM=$(TARGET_OS) AR="$(AR)" $(LEVELDB_TARGET_FLAGS) \
          OPT="$(CXXFLAGS) $(CPPFLAGS)"
endif

DOBBSCOIN_CONFIG_INCLUDES=-I$(builddir)/config
DOBBSCOIN_INCLUDES=-I$(builddir) -I$(builddir)/obj $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) $(CRYPTO_CFLAGS) $(SSL_CFLAGS)

DOBBSCOIN_INCLUDES += -I$(srcdir)/secp256k1/include

LIBDOBBSCOIN_SERVER=libdobbscoin_server.a
LIBDOBBSCOIN_WALLET=libdobbscoin_wallet.a
LIBDOBBSCOIN_COMMON=libdobbscoin_common.a
LIBDOBBSCOIN_CLI=libdobbscoin_cli.a
LIBDOBBSCOIN_UTIL=libdobbscoin_util.a
LIBDOBBSCOIN_CRYPTO=crypto/libdobbscoin_crypto.a
LIBDOBBSCOIN_UNIVALUE=univalue/libdobbscoin_univalue.a
LIBDOBBSCOINQT=qt/libdobbscoinqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la

$(LIBSECP256K1): $(wildcard secp256k1/src/*) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES = \
  crypto/libdobbscoin_crypto.a \
  libdobbscoin_util.a \
  libdobbscoin_common.a \
  univalue/libdobbscoin_univalue.a \
  libdobbscoin_server.a \
  libdobbscoin_cli.a
if ENABLE_WALLET
DOBBSCOIN_INCLUDES += $(BDB_CPPFLAGS)
EXTRA_LIBRARIES += libdobbscoin_wallet.a
endif

if BUILD_DOBBSCOIN_LIBS
lib_LTLIBRARIES = libdobbscoinconsensus.la
LIBDOBBSCOIN_CONSENSUS=libdobbscoinconsensus.la
else
LIBDOBBSCOIN_CONSENSUS=
endif

bin_PROGRAMS =
TESTS =

if BUILD_DOBBSCOIND
  bin_PROGRAMS += dobbscoind
endif

if BUILD_DOBBSCOIN_UTILS
  bin_PROGRAMS += dobbscoin-cli dobbscoin-tx
endif

.PHONY: FORCE
# dobbscoin core #
DOBBSCOIN_CORE_H = \
  addrman.h \
  alert.h \
  allocators.h \
  amount.h \
  auxpow.h \
  base58.h \
  bloom.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coincontrol.h \
  coins.h \
  compat.h \
  compressor.h \
  primitives/block.h \
  primitives/pureheader.h \
  primitives/transaction.h \
  core_io.h \
  crypter.h \
  db.h \
  eccryptoverify.h \
  ecwrapper.h \
  hash.h \
  init.h \
  key.h \
  keystore.h \
  leveldbwrapper.h \
  limitedmap.h \
  main.h \
  merkleblock.h \
  miner.h \
  mruset.h \
  netbase.h \
  net.h \
  noui.h \
  pow.h \
  protocol.h \
  pubkey.h \
  random.h \
  rpcclient.h \
  rpcprotocol.h \
  rpcserver.h \
  script/interpreter.h \
  script/script.h \
  script/sigcache.h \
  script/sign.h \
  script/standard.h \
  script/script_error.h \
  serialize.h \
  streams.h \
  sync.h \
  threadsafety.h \
  timedata.h \
  tinyformat.h \
  txdb.h \
  txmempool.h \
  ui_interface.h \
  uint256.h \
  undo.h \
  util.h \
  utilstrencodings.h \
  utilmoneystr.h \
  utiltime.h \
  version.h \
  wallet.h \
  wallet_ismine.h \
  walletdb.h \
  compat/sanity.h

JSON_H = \
  json/json_spirit.h \
  json/json_spirit_error_position.h \
  json/json_spirit_reader.h \
  json/json_spirit_reader_template.h \
  json/json_spirit_stream_reader.h \
  json/json_spirit_utils.h \
  json/json_spirit_value.h \
  json/json_spirit_writer.h \
  json/json_spirit_writer_template.h

obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh $(abs_top_builddir)/src/obj/build.h \
	  $(abs_top_srcdir)
libdobbscoin_util_a-clientversion.$(OBJEXT): obj/build.h

# server: shared between dobbscoind and dobbscoin-qt
libdobbscoin_server_a_CPPFLAGS = $(DOBBSCOIN_INCLUDES) $(MINIUPNPC_CPPFLAGS)
libdobbscoin_server_a_SOURCES = \
  addrman.cpp \
  alert.cpp \
  bloom.cpp \
  chain.cpp \
  checkpoints.cpp \
  init.cpp \
  leveldbwrapper.cpp \
  main.cpp \
  merkleblock.cpp \
  miner.cpp \
  net.cpp \
  noui.cpp \
  pow.cpp \
  rest.cpp \
  rpcblockchain.cpp \
  rpcmining.cpp \
  rpcmisc.cpp \
  rpcnet.cpp \
  rpcrawtransaction.cpp \
  rpcserver.cpp \
  script/sigcache.cpp \
  timedata.cpp \
  txdb.cpp \
  txmempool.cpp \
  $(JSON_H) \
  $(DOBBSCOIN_CORE_H)

# wallet: shared between dobbscoind and dobbscoin-qt, but only linked
# when wallet enabled
libdobbscoin_wallet_a_CPPFLAGS = $(DOBBSCOIN_INCLUDES)
libdobbscoin_wallet_a_SOURCES = \
  db.cpp \
  crypter.cpp \
  rpcdump.cpp \
  rpcwallet.cpp \
  wallet.cpp \
  wallet_ismine.cpp \
  walletdb.cpp \
  $(DOBBSCOIN_CORE_H)

# crypto primitives library
crypto_libdobbscoin_crypto_a_CPPFLAGS = $(DOBBSCOIN_CONFIG_INCLUDES)
crypto_libdobbscoin_crypto_a_SOURCES = \
  crypto/sha1.cpp \
  crypto/sha256.cpp \
  crypto/scrypt.cpp \
  crypto/sha512.cpp \
  crypto/hmac_sha256.cpp \
  crypto/rfc6979_hmac_sha256.cpp \
  crypto/hmac_sha512.cpp \
  crypto/ripemd160.cpp \
  crypto/common.h \
  crypto/sha256.h \
  crypto/sha512.h \
  crypto/scrypt.h \
  crypto/hmac_sha256.h \
  crypto/rfc6979_hmac_sha256.h \
  crypto/hmac_sha512.h \
  crypto/sha1.h \
  crypto/ripemd160.h

# univalue JSON library
univalue_libdobbscoin_univalue_a_SOURCES = \
  univalue/univalue.cpp \
  univalue/univalue_read.cpp \
  univalue/univalue_write.cpp \
  univalue/univalue_escapes.h \
  univalue/univalue.h

# common: shared between dobbscoind, and dobbscoin-qt and non-server tools
libdobbscoin_common_a_CPPFLAGS = $(DOBBSCOIN_INCLUDES)
libdobbscoin_common_a_SOURCES = \
  allocators.cpp \
  amount.cpp \
  auxpow.cpp \
  base58.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  primitives/block.cpp \
  primitives/pureheader.cpp \
  primitives/transaction.cpp \
  core_read.cpp \
  core_write.cpp \
  eccryptoverify.cpp \
  ecwrapper.cpp \
  hash.cpp \
  key.cpp \
  keystore.cpp \
  netbase.cpp \
  protocol.cpp \
  pubkey.cpp \
  script/interpreter.cpp \
  script/script.cpp \
  script/sign.cpp \
  script/standard.cpp \
  script/script_error.cpp \
  $(DOBBSCOIN_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libdobbscoin_util_a_CPPFLAGS = $(DOBBSCOIN_INCLUDES)
libdobbscoin_util_a_SOURCES = \
  compat/strnlen.cpp \
  compat/glibc_sanity.cpp \
  compat/glibcxx_sanity.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  random.cpp \
  rpcprotocol.cpp \
  sync.cpp \
  uint256.cpp \
  util.cpp \
  utilstrencodings.cpp \
  utilmoneystr.cpp \
  utiltime.cpp \
  $(DOBBSCOIN_CORE_H)

if GLIBC_BACK_COMPAT
libdobbscoin_util_a_SOURCES += compat/glibc_compat.cpp
libdobbscoin_util_a_SOURCES += compat/glibcxx_compat.cpp
endif

# cli: shared between dobbscoin-cli and dobbscoin-qt
libdobbscoin_cli_a_CPPFLAGS = $(DOBBSCOIN_INCLUDES)
libdobbscoin_cli_a_SOURCES = \
  rpcclient.cpp \
  $(DOBBSCOIN_CORE_H)

nodist_libdobbscoin_util_a_SOURCES = $(srcdir)/obj/build.h
#

# dobbscoind binary #
dobbscoind_LDADD = \
  $(LIBDOBBSCOIN_SERVER) \
  $(LIBDOBBSCOIN_COMMON) \
  $(LIBDOBBSCOIN_UNIVALUE) \
  $(LIBDOBBSCOIN_UTIL) \
  $(LIBDOBBSCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBMEMENV) \
  $(LIBSECP256K1)

if ENABLE_WALLET
dobbscoind_LDADD += libdobbscoin_wallet.a
endif
dobbscoind_SOURCES = dobbscoind.cpp
#

if TARGET_WINDOWS
dobbscoind_SOURCES += dobbscoind-res.rc
endif

dobbscoind_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS)
dobbscoind_CPPFLAGS = $(DOBBSCOIN_INCLUDES)
dobbscoind_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

# dobbscoin-cli binary #
dobbscoin_cli_LDADD = \
  $(LIBDOBBSCOIN_CLI) \
  $(LIBDOBBSCOIN_UTIL) \
  $(BOOST_LIBS) \
  $(SSL_LIBS) \
  $(CRYPTO_LIBS)

dobbscoin_cli_SOURCES = \
  dobbscoin-cli.cpp

dobbscoin_cli_CPPFLAGS = $(DOBBSCOIN_INCLUDES)
#

# dobbscoin-tx binary #
dobbscoin_tx_LDADD = \
  $(LIBDOBBSCOIN_UNIVALUE) \
  $(LIBDOBBSCOIN_COMMON) \
  $(LIBDOBBSCOIN_UTIL) \
  $(LIBDOBBSCOIN_CRYPTO) \
  $(LIBSECP256K1) \
  $(BOOST_LIBS) \
  $(CRYPTO_LIBS)

dobbscoin_tx_SOURCES = dobbscoin-tx.cpp
dobbscoin_tx_CPPFLAGS = $(DOBBSCOIN_INCLUDES)
#
dobbscoin_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
dobbscoin_cli_SOURCES += dobbscoin-cli-res.rc
endif
dobbscoin_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if BUILD_DOBBSCOIN_LIBS
include_HEADERS = script/dobbscoinconsensus.h
libdobbscoinconsensus_la_SOURCES = \
  primitives/transaction.cpp \
  crypto/hmac_sha512.cpp \
  crypto/sha1.cpp \
  crypto/sha256.cpp \
  crypto/scrypt.cpp \
  crypto/sha512.cpp \
  crypto/ripemd160.cpp \
  eccryptoverify.cpp \
  ecwrapper.cpp \
  hash.cpp \
  pubkey.cpp \
  script/script.cpp \
  script/interpreter.cpp \
  script/dobbscoinconsensus.cpp \
  uint256.cpp \
  utilstrencodings.cpp

if GLIBC_BACK_COMPAT
  libdobbscoinconsensus_la_SOURCES += compat/glibc_compat.cpp
  libdobbscoinconsensus_la_SOURCES += compat/glibcxx_compat.cpp
endif

libdobbscoinconsensus_la_LDFLAGS = -no-undefined $(RELDFLAGS)
libdobbscoinconsensus_la_LIBADD = $(CRYPTO_LIBS)
libdobbscoinconsensus_la_CPPFLAGS = $(CRYPTO_CFLAGS) -I$(builddir)/obj -DBUILD_DOBBSCOIN_INTERNAL
if USE_LIBSECP256K1
libdobbscoinconsensus_la_LIBADD += secp256k1/libsecp256k1.la
endif
endif

CLEANFILES = leveldb/libleveldb.a leveldb/libmemenv.a *.gcda *.gcno

DISTCLEANFILES = obj/build.h

EXTRA_DIST = leveldb

clean-local:
	-$(MAKE) -C leveldb clean
	-$(MAKE) -C secp256k1 clean
	rm -f leveldb/*/*.gcno leveldb/helpers/memenv/*.gcno
	-rm -f config.h

.rc.o:
	@test -f $(WINDRES)
	$(AM_V_GEN) $(WINDRES) -DWINDRES_PREPROC -i $< -o $@

.mm.o:
	$(AM_V_CXX) $(OBJCXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	  $(CPPFLAGS) $(AM_CXXFLAGS) $(QT_INCLUDES) $(CXXFLAGS)  -c -o $@ $<

%.pb.cc %.pb.h: %.proto
	@test -f $(PROTOC)
	$(AM_V_GEN) $(PROTOC) --cpp_out=$(@D) --proto_path=$(abspath $(<D) $<)

if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif
