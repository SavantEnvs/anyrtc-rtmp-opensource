# rtmp_client_input casts an out-of-range AMF number to `uint32_t`

`input.bin` delivers an AMF0 `_result` command whose transaction id is a huge double (about -6.5e303).
`librtmp/source/rtmp-client-invoke-handler.h:93` (`rtmp_command_onresult`) does `switch ((uint32_t)transaction)`;
the conversion is undefined behaviour for values outside the `uint32_t` range (UBSan `float-cast-overflow`).

Impact: undefined behaviour, in practice a wrong transaction id and an ignored reply. Fix: range-check the double before the cast.
