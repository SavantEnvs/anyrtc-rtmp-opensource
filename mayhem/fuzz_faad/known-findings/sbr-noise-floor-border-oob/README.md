# FAAD2 SBR `noise_floor_time_border_vector()` indexes `t_E` out of bounds

`input.bin` is a crafted raw AAC frame with an SBR extension. `libfaad/sbr_tf_grid.c:169` reads `sbr->t_E[ch][index]`
(`uint8_t[6]`) with index 6: the index comes from `middleBorder()`, which is computed from SBR grid fields read from the stream and is not
range-checked against `t_E`.

Impact: out-of-bounds read on a crafted SBR stream. Fix: validate the index (and `L_E`) against the `t_E` size.
