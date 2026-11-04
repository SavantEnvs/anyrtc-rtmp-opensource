# FAAD2 `huffman_getescape()` shifts by an unbounded escape size

`input.bin` is a crafted raw AAC frame. In `libfaad/huffman.c:153` the escape-size loop `for (i = 4; ; i++)` keeps
counting while the stream has 1-bits, with no upper bound, and the result is used as `1<<i` (exponent 34..49 seen).
The preceding `faad_getbits(ld, i)` is asked for more than 32 bits as well.

Impact: undefined behaviour (shift exponent >= 32) and a wrong spectral value on a crafted stream. Fix: bound `i` (ISO 14496-3 allows an escape size of at most 16) and return an error.
