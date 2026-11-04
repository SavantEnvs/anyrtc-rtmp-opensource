# FAAD2 `output_to_PCM()` reads a channel buffer that was never allocated

`input.bin` is mode byte `31` (`NeAACDecInit2`) with an AudioSpecificConfig of object type 17 (ER AAC LC), 44.1 kHz and
channelConfiguration 6 (5.1), followed by a raw frame that contains fewer elements than the configuration promises.
`NeAACDecInit2` and the frame decode report 6 output channels (`fr_channels == 6`), but only channel 0's `time_out[]` buffer is
allocated; `output_to_PCM` (`output.c:415` -> `to_PCM_16bit:143`) then calls `get_sample()` for channel 1 and
`libfaad/output.c:49` indexes `input[internal_channel[1]]`, which is NULL (UBSan: "applying zero offset to null pointer";
without UBSan it is a SEGV on the next load).

Impact: NULL dereference (crash) on a crafted AAC stream. Same family as `ifilter-bank-null-overlap` (a per-channel buffer that does
not exist for a channel the decoder reports), at a different site. Fix: only emit channels that were decoded, or allocate all
channels announced by the configuration.
