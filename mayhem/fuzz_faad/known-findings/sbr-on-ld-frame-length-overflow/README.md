# FAAD2 SBR on an AAC-LD stream reads and writes past the PCM buffers (heap overflow)

Two reproducers, one root cause. Both are mode byte `31`/`01` (`NeAACDecInit2`) with an AudioSpecificConfig of object
type 23 (ER AAC-LD) followed by a raw frame carrying SBR:
- `input-synthesis-write.bin`: `libfaad/sbr_qmf.c:614` (`sbr_qmf_synthesis_64`) writes 0 bytes past a 4096-byte
  `time_out[]` allocation (`specrec.c:736`, `mul*frameLength*sizeof(real_t)` = 2*512*4). ASan: heap-buffer-overflow WRITE.
- `input-analysis-read.bin`: `libfaad/sbr_qmf.c:96` (`sbr_qmf_analysis_32`, reached from `sbrDecodeCoupleFrame`) reads 0 bytes
  past a 3840-byte `time_out[]` allocation (`specrec.c:836`, 2*480*4). ASan: heap-buffer-overflow READ.

Root cause: for ER-LD, `decoder.c:359/456` halves `frameLength` to 512/480, and `time_out[]` is sized `2*frameLength` when SBR is on
(`forceUpSampling` for rates <= 24 kHz, or `sbr_present_flag`). The SBR code does not scale with the frame length:
`sbr_dec.c:98-102` always uses `numTimeSlotsRate = 2*16` (or 2*15) slots of 32/64 samples, i.e. it consumes 1024/960 samples
in the analysis QMF and produces 2048/1920 samples in the synthesis QMF per frame. The PCM buffers are half that.

Impact: heap out-of-bounds write (up to 1024 floats, 4 KiB, of decoded samples) and read on a crafted AAC-LD+SBR stream. Fix: reject SBR for object types whose frame length is not 1024/960, or size the buffers from
`numTimeSlotsRate*64`.
