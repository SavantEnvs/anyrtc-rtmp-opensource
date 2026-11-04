#!/usr/bin/env bash
#
# mayhem/build.sh — build the fuzz harnesses (sanitized) and the KAT oracle probe (normal flags).
# Runs in /mayhem as `mayhem`. Everything is compiled from in-tree sources only (air-gapped, re-runnable).
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

RTMP="$SRC/ArLiveLite/rtmp"
FAAD="$SRC/third_party/faad2-2.7"
B="$SRC/mayhem/build"            # gitignored (mayhem/.gitignore) so `git clean -ffdX` resets it
mkdir -p "$B/san" "$B/norm"

INCS="-I$RTMP/libflv/include -I$RTMP/librtmp/include -I$FAAD/include -I$FAAD/libfaad"
FAAD_SRCS="bits.c cfft.c decoder.c drc.c drm_dec.c error.c filtbank.c ic_predict.c is.c lt_predict.c \
  mdct.c mp4.c ms.c output.c pns.c ps_dec.c ps_syntax.c pulse.c specrec.c syntax.c tns.c hcr.c \
  huffman.c rvlc.c ssr.c ssr_fb.c ssr_ipqf.c common.c sbr_dct.c sbr_e_nf.c sbr_fbt.c sbr_hfadj.c \
  sbr_hfgen.c sbr_huff.c sbr_qmf.c sbr_syntax.c sbr_tf_grid.c sbr_dec.c"
FLV_SRCS=$(cd "$RTMP/libflv/source" && ls ./*.c | sed 's|^\./||' | grep -v '^riff-acm\.c$')
RTMP_SRCS=$(cd "$RTMP/librtmp/source" && ls ./*.c | sed 's|^\./||')

# cc_one <src> <obj> <flags...>
# rtmp-handshake.c stores an int through an odd byte offset on every handshake (benign on x86/arm64);
# UBSan alignment is relaxed on that one file so it does not abort every input.
cc_one() {
  local src="$1" obj="$2" extra=""; shift 2
  case "$src" in */rtmp-handshake.c) extra="-fno-sanitize=alignment" ;; esac
  "$CC" "$@" $extra $INCS -w -c "$src" -o "$obj"
}
export -f cc_one
export INCS CC

# compile_libs <outdir> <flags...>: libfaad + libflv + librtmp(client) objects into <outdir>
compile_libs() {
  local out="$1"; shift
  {
    for f in $FAAD_SRCS; do printf '%s\0%s\0' "$FAAD/libfaad/$f" "$out/faad_${f%.c}.o"; done
    for f in $FLV_SRCS;  do printf '%s\0%s\0' "$RTMP/libflv/source/$f" "$out/flv_${f%.c}.o"; done
    for f in $RTMP_SRCS; do printf '%s\0%s\0' "$RTMP/librtmp/source/$f" "$out/rtmp_${f%.c}.o"; done
  } | xargs -0 -n2 -P "$MAYHEM_JOBS" bash -c 'cc_one "$0" "$1" '"$*"
  rm -f "$out/libmayhem.a"
  ar rcs "$out/libmayhem.a" "$out"/*.o
}

# 1) Sanitized + SanCov-instrumented libraries (DWARF 3). fuzzer-no-link is unconditional so the
#    library code carries coverage instrumentation even when SANITIZER_FLAGS is empty.
# UBSan shift-base is relaxed on the libraries: `buf[0] << 24` (a uint8_t promoted to int) and shifts of
# negative values are the idiom throughout libflv/librtmp/faad2 and would abort on every byte >= 0x80
# long before any real bug is reached. Every other UBSan check and ASan stay halting.
LIBFLAGS="-O1 -DFAAD_HAVE_CONFIG_H $SANITIZER_FLAGS -fno-sanitize=shift-base -fsanitize=fuzzer-no-link $DEBUG_FLAGS"
rm -f "$B"/san/*.o
compile_libs "$B/san" $LIBFLAGS

# 2) Harnesses: libFuzzer binary + non-fuzzer -standalone reproducer, both without LeakSanitizer.
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$B/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$B/standalone_main.o"
for h in fuzz_faad fuzz_flv_parser fuzz_flv_demuxer fuzz_rtmp_client fuzz_amf0; do
  $CC -O1 $SANITIZER_FLAGS -fsanitize=fuzzer-no-link $DEBUG_FLAGS $INCS -w -c \
      "$SRC/mayhem/harnesses/$h.c" -o "$B/$h.o"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$B/$h.o" "$B/san/libmayhem.a" "$B/lsan_off.o" -lm \
      -o "/mayhem/$h"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS "$B/$h.o" "$B/standalone_main.o" "$B/san/libmayhem.a" "$B/lsan_off.o" -lm \
      -o "/mayhem/$h-standalone"
done

# 3) Oracle: known-answer probe built with the project's NORMAL flags (no sanitizers, no DWARF pin).
#    It is dynamically linked so the sabotage shim (LD_PRELOAD) can neuter it.
rm -f "$B"/norm/*.o
compile_libs "$B/norm" -O2 -DFAAD_HAVE_CONFIG_H $COVERAGE_FLAGS
$CC -O2 $COVERAGE_FLAGS $INCS -w "$SRC/mayhem/kat.c" "$B/norm/libmayhem.a" -lm -o "$B/kat"
