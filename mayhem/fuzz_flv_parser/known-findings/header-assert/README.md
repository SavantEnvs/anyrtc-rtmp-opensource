# flv_parser_input aborts on a malformed FLV header / tag

The parser validates its input with `assert()`: signature and flags (`flv-header.c:27`, `:36`), tag type
(`flv-header.c:55`), audio/video packet type (`flv-header.c:86`, `:110`) and the PreviousTagSize field
(`flv-parser.c:145`). `input.bin` (first byte = split point, then a bad 8-byte header) trips the first.

Impact: remote abort (DoS) when FLV bytes arrive from an untrusted peer. Fix: return an error code instead of asserting.
