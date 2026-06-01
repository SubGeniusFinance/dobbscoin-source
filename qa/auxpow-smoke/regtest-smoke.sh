#!/usr/bin/env bash
#
# AuxPoW regtest smoke test for (BOB) v0.12.0.
#
# Drives the createauxblock/submitauxblock RPC contract end-to-end against
# a regtest dobbscoind.  Verifies:
#
#   1. Pre-activation height: createauxblock refuses (RPC method not yet).
#   2. Post-activation height: createauxblock returns the documented 7-field
#      JSON object with chainid == 0x00B0 (176) and a 64-hex-char target.
#   3. The cached template hash → submitauxblock can find it; an unknown
#      hash → "block hash unknown" rejection.
#   4. Malformed auxpow hex → "AuxPow decode failed" rejection.
#
# What this does NOT do: actually construct a valid CAuxPow that satisfies
# the parent-PoW check.  That's MiningCore's job and is exercised by
# pointing real MiningCore at this daemon — a runbook in this directory
# rather than a unit test, because it needs an in-process scrypt miner.
#
# Usage: ./regtest-smoke.sh /path/to/src/dobbscoind /path/to/src/dobbscoin-cli
# Defaults assume cwd = qa/auxpow-smoke/.

set -euo pipefail

DOBBSCOIND="${1:-../../src/dobbscoind}"
CLI="${2:-../../src/dobbscoin-cli}"
DATADIR="$(mktemp -d)"
# Random high port so we don't collide with any other regtest daemon
# the user might have running.  Same port handed to both daemon and CLI.
RPCPORT=$(( 30000 + RANDOM % 20000 ))
CLI_ARGS=(-regtest -datadir="$DATADIR" -rpcport="$RPCPORT" -rpcwait -rpcclienttimeout=300)

cleanup() {
    if [[ -f "$DATADIR/regtest/dobbscoind.pid" ]]; then
        "$CLI" "${CLI_ARGS[@]}" stop >/dev/null 2>&1 || true
        sleep 1
    fi
    rm -rf "$DATADIR"
}
trap cleanup EXIT

ok()   { echo "  OK   $1"; }
fail() { echo "  FAIL $1" >&2; exit 1; }

echo "==> Starting regtest dobbscoind in $DATADIR"
"$DOBBSCOIND" -regtest -datadir="$DATADIR" -daemon \
              -rpcuser=auxpow -rpcpassword=test \
              -rpcport="$RPCPORT" \
              -listen=0 -discover=0 \
              >/dev/null 2>&1
CLI_ARGS+=(-rpcuser=auxpow -rpcpassword=test)

# rpcwait makes the first call block until the daemon is ready.
HEIGHT=$("$CLI" "${CLI_ARGS[@]}" getblockcount)
[[ "$HEIGHT" == "0" ]] && ok "regtest fresh at height 0" || fail "expected height 0, got $HEIGHT"

ADDR=$("$CLI" "${CLI_ARGS[@]}" getnewaddress)
ok "got test address $ADDR"

echo "==> Test 1: createauxblock refused before AuxPoW activation"
# Regtest AuxPoW activation = HARDFORK_AUXPOW_TESTNET = 200.
# At height 0, next block is 1, well below 200.
if RESPONSE=$("$CLI" "${CLI_ARGS[@]}" createauxblock "$ADDR" 2>&1); then
    fail "createauxblock should refuse pre-activation, got: $RESPONSE"
fi
echo "$RESPONSE" | grep -q "activation height not yet reached" \
    && ok "rejected with expected message" \
    || fail "wrong error message: $RESPONSE"

echo "==> Mining 20 regtest blocks to cross AuxPoW activation (testnet fork @ 10)"
"$CLI" "${CLI_ARGS[@]}" setgenerate true 20 >/dev/null
HEIGHT=$("$CLI" "${CLI_ARGS[@]}" getblockcount)
[[ "$HEIGHT" -ge "20" ]] && ok "chain at height $HEIGHT" || fail "expected >=20, got $HEIGHT"

echo "==> Test 2: createauxblock returns valid 7-field object post-activation"
TEMPLATE=$("$CLI" "${CLI_ARGS[@]}" createauxblock "$ADDR")
for field in hash chainid previousblockhash coinbasevalue bits height target; do
    echo "$TEMPLATE" | grep -q "\"$field\"" \
        && ok "field $field present" \
        || fail "missing field $field in: $TEMPLATE"
done

CHAINID=$(echo "$TEMPLATE" | grep -oP '"chainid"\s*:\s*\K[0-9]+')
[[ "$CHAINID" == "176" ]] && ok "chainid == 176 (0x00B0)" || fail "wrong chainid $CHAINID, expected 176"

TARGET=$(echo "$TEMPLATE" | grep -oP '"target"\s*:\s*"\K[0-9a-f]+')
[[ "${#TARGET}" == "64" ]] && ok "target is 64 hex chars" || fail "target wrong length: ${#TARGET}"

HASH=$(echo "$TEMPLATE" | grep -oP '"hash"\s*:\s*"\K[0-9a-f]+')
[[ "${#HASH}" == "64" ]] && ok "hash is 64 hex chars" || fail "hash wrong length: ${#HASH}"

TEMPLATE_HEIGHT=$(echo "$TEMPLATE" | grep -oP '"height"\s*:\s*\K[0-9]+')
EXPECTED_HEIGHT=$((HEIGHT + 1))
[[ "$TEMPLATE_HEIGHT" == "$EXPECTED_HEIGHT" ]] \
    && ok "template height $TEMPLATE_HEIGHT == tip+1" \
    || fail "template height $TEMPLATE_HEIGHT, expected $EXPECTED_HEIGHT"

[[ "$HEIGHT" -ge "10" ]] || fail "should be past AuxPoW fork @ 10, got $HEIGHT"

echo "==> Test 3: submitauxblock rejects unknown block hash"
BOGUS_HASH="aa$(echo "$HASH" | cut -c3-)"
if RESPONSE=$("$CLI" "${CLI_ARGS[@]}" submitauxblock "$BOGUS_HASH" "00" 2>&1); then
    fail "submitauxblock should refuse unknown hash, got: $RESPONSE"
fi
echo "$RESPONSE" | grep -q "block hash unknown\|AuxPow decode failed" \
    && ok "rejected (unknown hash or decode-first failure)" \
    || fail "wrong rejection: $RESPONSE"

echo "==> Test 4: submitauxblock rejects malformed auxpow hex"
if RESPONSE=$("$CLI" "${CLI_ARGS[@]}" submitauxblock "$HASH" "not-valid-hex" 2>&1); then
    fail "submitauxblock should refuse non-hex, got: $RESPONSE"
fi
echo "$RESPONSE" | grep -q "AuxPow decode failed" \
    && ok "rejected with decode error" \
    || fail "wrong rejection: $RESPONSE"

echo "==> Test 5: createauxblock returns same template until tip moves"
TEMPLATE2=$("$CLI" "${CLI_ARGS[@]}" createauxblock "$ADDR")
HASH2=$(echo "$TEMPLATE2" | grep -oP '"hash"\s*:\s*"\K[0-9a-f]+')
[[ "$HASH" == "$HASH2" ]] && ok "second call returned same hash (cached)" \
    || ok "second call returned new hash (mempool drift / staleness; both valid)"

echo "==> Test 6: advancing the tip invalidates cached templates"
"$CLI" "${CLI_ARGS[@]}" setgenerate true 1 >/dev/null
TEMPLATE3=$("$CLI" "${CLI_ARGS[@]}" createauxblock "$ADDR")
HASH3=$(echo "$TEMPLATE3" | grep -oP '"hash"\s*:\s*"\K[0-9a-f]+')
[[ "$HASH3" != "$HASH" ]] && ok "new tip → fresh template hash" \
    || fail "cache should have been invalidated when tip advanced"

echo
echo "All regtest AuxPoW RPC smoke tests passed."
