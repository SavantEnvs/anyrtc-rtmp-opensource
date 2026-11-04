# FAAD2 `section_data()` writes past the 120-entry section arrays

`input.bin` is a crafted raw AAC frame. `libfaad/syntax.c:1693` stores `ics->sect_cb[g][i]` with `i == 120`,
out of bounds for `uint8_t[120]`: the section index `i` is never checked against the array size.

Impact: out-of-bounds write into the `ic_stream` struct on a crafted AAC stream. Fix: stop with an error once `i >= 15*8` (the array bound).
