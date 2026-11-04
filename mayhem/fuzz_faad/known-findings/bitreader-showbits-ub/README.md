# FAAD2 bit reader: `faad_showbits()` overflows with a bad bit count

Two reproducers, both at `libfaad/bits.h:153` (`faad_showbits`, reached through `faad_getbits`):
- `input-shift-exponent.bin`: column 49, `ld->bufb >> (32 - bits)` with `bits` > 32 (exponent 34 seen). A caller passes a
  stream-derived bit count with no bound.
- `input-signed-overflow.bin`: column 44, `((1 << ld->bits_left) - 1)` evaluates `INT_MIN - 1` when `bits_left` is 32.

The second one is benign on every compiler we know (two's complement wrap), the first yields garbage bits.

Impact: undefined behaviour on a crafted stream, no memory corruption observed. Fix: use `1U`/a mask table in `faad_showbits` and clamp `bits` to 32.
