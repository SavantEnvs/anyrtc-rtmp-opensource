# rtmp_client_input aborts on a malformed protocol control message

Four reproducers in `librtmp/source/rtmp-control-handler.c` (`rtmp_control_handler`):
- `input-cid.bin`: line 79, `assert(2 == header->cid)`. Control message types (set chunk size, ack, ...) arrive on a
  chunk stream other than 2.
- `input-length.bin`: line 97, `assert(4 == header->length)` for an Acknowledgement whose length is not 4.
- `input-window-ack-length.bin`: line 101, `assert(4 == header->length)` for a Window Acknowledgement Size message
  (type 5) whose length is 2.
- `input-peer-bandwidth-length.bin`: line 105, `assert(5 == header->length)` for a Set Peer Bandwidth message
  (type 6) whose length is not 5.

The same pattern guards set-chunk-size and abort at lines 84 and 88 (`assert(4 == header->length)`), and the `default:` branch at line 115
(`assert(0)` on an unknown control message type). Those three sites have no reproducer yet. They share the root cause: the
message length and chunk-stream id come straight from the peer's chunk header and are only `assert`ed.

Impact: remote abort (DoS) of an RTMP client by its server. Fix: return an error instead of asserting on cid/length/type.
