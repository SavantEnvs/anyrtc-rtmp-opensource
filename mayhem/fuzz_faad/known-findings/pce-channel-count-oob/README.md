# FAAD2 program_config_element accepts more than MAX_CHANNELS channels

`input.bin` is mode byte `01` (`NeAACDecInit2`) with channelConfiguration 0, so the channel layout comes from a Program Config
Element (`pce_set == 1`). The PCE element counts (up to 15 front, 15 side, 15 back, each possibly a channel pair, plus 15 LFE) are summed into `pce->channels`
(`program_config_element`, `syntax.c:225-295`) without checking it against `MAX_CHANNELS` (64, `structs.h:43`). In this reproducer `pce.channels == 66` and
`pce.sce_channel[0] == 65`.

`libfaad/syntax.c:361` (`decode_sce_lfe`) then executes `hDecoder->internal_channel[hDecoder->pce.sce_channel[tag]] = channels`,
an out-of-bounds write into `uint8_t internal_channel[64]` (UBSan: "index 65 out of bounds for type 'uint8_t[64]'"). The same
unchecked value feeds the `cpe_channel[]` stores at `syntax.c:407-408`.

Impact: out-of-bounds write of a one-byte channel index inside the decoder structure (past `internal_channel`) on a crafted AAC stream. Fix: reject a PCE whose `channels` exceeds `MAX_CHANNELS`.
