# FAAD2 SBR `patch_construction()` underflows a `uint8_t` loop index

`input.bin` is mode byte `76` (ADTS/ADIF stream) with a crafted SBR extension. The decoder ends up with `k0 = 10`, `kx = 27`,
`M = 37`, `N_master = 10` and `f_master[0] = 27`. `goalSb` (from the sample-rate table `goalSbTab`) is not above `f_master[0]`, so
the first loop in `patch_construction` (`sbr_hfgen.c:614-618`) leaves `k = 0`.

The inner `do { j--; sb = sbr->f_master[j]; ... } while (sb > (sbr->k0 - 1 + msb - odd))` at `libfaad/sbr_hfgen.c:636-641`
(reached via `hf_generation`, `sbr_hfgen.c:79`) has no lower bound on `j`. `f_master[0] = 27` already satisfies the loop condition
(27 > 10 - 1 + 10 - odd), so `j` (a `uint8_t`) wraps from 0 to 255 and `f_master[255]` is read from `uint8_t f_master[64]`
(UBSan: "index 255 out of bounds").

Impact: out-of-bounds read of decoder-structure memory used to derive the patch layout, on a crafted SBR stream. Fix: bound `j`
(stop at 0) and reject headers whose `f_master[0]` is above `goalSb`.
