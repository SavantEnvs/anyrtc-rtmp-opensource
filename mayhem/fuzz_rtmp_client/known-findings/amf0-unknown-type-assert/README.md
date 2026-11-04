# AMF0 decoder aborts on an unknown type marker (reached through the RTMP client)

`input.bin` is a handshake plus an RTMP command message whose AMF0 payload contains an unknown type marker.
`libflv/source/amf0.c:331` (`amf_read_item`) falls into `default: assert(0)`. Same root cause as the
`fuzz_amf0` finding `unknown-type-assert`, reached here through `rtmp_client_input`.

Impact: remote abort (DoS) of an RTMP client by its server. Fix: return NULL (error) instead of asserting.
