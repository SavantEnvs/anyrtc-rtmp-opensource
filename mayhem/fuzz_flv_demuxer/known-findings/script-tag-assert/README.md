# flv_demuxer_script aborts on a malformed script (AMF0) tag body

`input.bin` is one record: kind 2 (script tag), length `0xffff` (clamped to the remaining bytes, none).
`libflv/source/flv-demuxer-script.c:64` hits `assert(0)` when the script tag body does not start with an
AMF string (here the body is empty, and `data[0]` is read before any length check). Related aborts found by the fuzzer: `amf0.c:331`, `mpeg4-mp4toannexb.c:49`/`:92`,
`mpeg4-avc.c:55`/`:99`, `mpeg4-aac.c:113`, `flv-demuxer.c:151`, `flv-header.c:86`/`:110`.

Impact: remote abort (DoS) on any FLV stream with a malformed tag. Fix: return an error code instead of asserting.
