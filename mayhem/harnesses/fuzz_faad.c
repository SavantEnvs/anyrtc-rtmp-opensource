// Fuzzes the bundled FAAD2 AAC decoder (third_party/faad2-2.7): stream init + frame decode.
// byte 0 bit 0 selects the entry point: 1 = NeAACDecInit2 (2-byte AudioSpecificConfig, raw
// frames follow, as delivered by FLV/RTMP), 0 = NeAACDecInit (ADTS/ADIF stream).
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "neaacdec.h"

#define MAX_FRAMES 16

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 4)
        return -1;

    unsigned char mode = data[0];
    unsigned char *buf = (unsigned char *)(data + 1);
    unsigned long left = (unsigned long)(size - 1);
    unsigned long samplerate = 0;
    unsigned char channels = 0;

    NeAACDecHandle h = NeAACDecOpen();
    if (!h)
        return -1;

    if (mode & 1) {
        if (NeAACDecInit2(h, buf, 2, &samplerate, &channels) < 0) {
            NeAACDecClose(h);
            return -1;
        }
        buf += 2;
        left -= 2;
    } else {
        long skip = NeAACDecInit(h, buf, left, &samplerate, &channels);
        if (skip < 0 || (unsigned long)skip > left) {
            NeAACDecClose(h);
            return -1;
        }
        buf += skip;
        left -= (unsigned long)skip;
    }

    volatile unsigned sink = 0;
    for (int i = 0; i < MAX_FRAMES && left > 0; i++) {
        NeAACDecFrameInfo info;
        memset(&info, 0, sizeof(info));
        short *pcm = (short *)NeAACDecDecode(h, &info, buf, left);
        if (pcm && info.samples > 0)
            sink += (unsigned short)pcm[0] + (unsigned short)pcm[info.samples - 1];
        if (info.bytesconsumed == 0 || info.bytesconsumed > left)
            break;
        buf += info.bytesconsumed;
        left -= info.bytesconsumed;
    }

    NeAACDecClose(h);
    return 0;
}
