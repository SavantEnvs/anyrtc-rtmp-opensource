# rtmp_client_input aborts when a message's length changes between chunks

`input.bin` sends a chunked message and then a continuation chunk whose header (message length) disagrees with the message in
progress. In `librtmp/source/rtmp-chunk-read.c:214` (`rtmp_chunk_read`, state `RTMP_PARSE_PAYLOAD`) the bookkeeping check
on `pkt->bytes` / `header.length` / `capacity` fails and the code runs `assert(0)` right before its `return ENOMEM`.

Impact: remote abort (DoS) of an RTMP client by its server. Fix: drop the `assert(0)`; the error return right below it already handles the case.
