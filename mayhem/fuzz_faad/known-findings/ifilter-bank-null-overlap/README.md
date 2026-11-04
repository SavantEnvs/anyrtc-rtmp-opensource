# FAAD2 `ifilter_bank()` runs with a NULL overlap buffer

`input.bin` (mode byte `01`, `NeAACDecInit2` AudioSpecificConfig, then a raw frame) is accepted by `NeAACDecInit2`, but while
decoding the first frame the overlap buffer of the channel (`hDecoder->fb_intermed[sce->channel]`, passed to `ifilter_bank`
from `specrec.c:1014`) is NULL. `libfaad/filtbank.c:223` reads `overlap[i]` on it (UBSan reports "applying zero offset to
null pointer" before the NULL load, which would crash anyway).

Impact: NULL dereference (crash) on a crafted AAC stream. Fix: allocate the overlap buffer for every channel the decoder can emit, or reject the element.
