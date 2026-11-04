# rtmp_client_input aborts on an unexpected `onStatus` level or code

Two reproducers in `librtmp/source/rtmp-client-invoke-handler.h` (`rtmp_command_onstatus`), delivered as AMF0 command messages
after the handshake:
- `input-level.bin`: line 165, the `level` string is not one of `error`/`status`/`warning`/`finish`.
- `input-code.bin`: line 219, `code` is a NetStream status the handler does not know, so it falls into the final `assert(0)`.

Impact: remote abort (DoS) of an RTMP client by its server. Fix: log and return an error instead of asserting.
