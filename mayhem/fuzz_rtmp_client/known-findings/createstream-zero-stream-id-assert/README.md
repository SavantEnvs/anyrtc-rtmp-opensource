# rtmp_client_input aborts when the createStream `_result` carries stream id 0

`input.bin` is a server->client chunk stream (after a valid S0/S1/S2 handshake) with a `connect` reply followed by a
`_result` for transaction 2 (`createStream`) whose result is the AMF0 number `0.0002` (`0x3f2b000000000000`).

`rtmp_command_onresult` (`rtmp-client-invoke-handler.h:108`) passes it to `rtmp_client_oncreate_stream`, which stores
`ctx->stream_id = (uint32_t)stream_id` (`rtmp-client.c:230`), i.e. 0, and immediately sends `play`: `rtmp_client_send_play`
runs `assert(0 != ctx->stream_id)` at `librtmp/source/rtmp-client.c:163`. The same stream id feeds the sibling asserts in
`rtmp_client_send_publish` and the other stream-bound senders (`rtmp-client.c:145`, `:154`, `:536`, `:551`, `:566`).

Impact: remote abort (DoS) of an RTMP client by its server. Fix: validate the stream id in `rtmp_client_oncreate_stream` and
return an error instead of asserting.
