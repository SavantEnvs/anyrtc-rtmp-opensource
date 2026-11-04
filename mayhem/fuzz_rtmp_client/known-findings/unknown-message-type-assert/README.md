# rtmp_client_input aborts on an unknown RTMP message type id

After the (valid) handshake that the harness sends, `input.bin` is the step byte `ff` followed by one chunk
whose message type id is not a known RTMP type. `librtmp/source/rtmp-handler.c:73` hits `assert(0)` for an unknown
message type.

Impact: remote abort (DoS) of any RTMP player connecting to a hostile server. Fix: return an error instead of asserting.
