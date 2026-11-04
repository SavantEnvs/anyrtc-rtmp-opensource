#!/usr/bin/env bash
#
# mayhem/test.sh — behavioral oracle. Runs the known-answer probe (mayhem/kat.c) that mayhem/build.sh
# built with the project's normal flags (no sanitizers) and compares its output, line by line, with
# mayhem/kat.expected: AMF0 command decoding, FLV tag parsing, FAAD2 AAC-LC init + decode, RTMP client
# chunk reassembly. A program that does nothing (or exits 0 early) cannot produce those lines.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$skipped" "$pending" "$other"
  [ "$failed" -eq 0 ]
}

KAT="$SRC/mayhem/build/kat"
EXPECTED="$SRC/mayhem/kat.expected"
if [ ! -x "$KAT" ] || [ ! -f "$EXPECTED" ]; then
  echo "missing $KAT or $EXPECTED — mayhem/build.sh must build the probe first" >&2
  emit_ctrf "anyrtc-kat" 0 1 0
  exit $?
fi

out="$("$KAT" 2>&1)"
echo "$out"

passed=0
failed=0
for prefix in amf flv faad rtmp; do
  want="$(grep "^$prefix " "$EXPECTED")"
  got="$(printf '%s\n' "$out" | grep "^$prefix ")"
  if [ -n "$want" ] && [ "$want" = "$got" ]; then
    passed=$((passed + 1))
  else
    failed=$((failed + 1))
    echo "FAIL $prefix: expected:" >&2
    printf '%s\n' "$want" >&2
    echo "got:" >&2
    printf '%s\n' "$got" >&2
  fi
done

emit_ctrf "anyrtc-kat" "$passed" "$failed" 0
