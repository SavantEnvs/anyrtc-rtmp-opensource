# FAAD2 indexes sampling-rate tables with an unchecked 4-bit index

`input.bin` is mode byte `00` (ADTS) followed by an ADTS header whose sampling_frequency_index is 15.
`libfaad/specrec.c:326` (`window_grouping_info`) reads `const uint8_t[12]` out of bounds. `input-short-window.bin` reaches the
same table through the `EIGHT_SHORT_SEQUENCE` branch: `specrec.c:378` (`num_swb_128_window[sf_index]`, index 15).

Impact: out-of-bounds read / undefined behaviour on a crafted AAC stream. Fix: reject sf_index > 11 where the header is parsed.
