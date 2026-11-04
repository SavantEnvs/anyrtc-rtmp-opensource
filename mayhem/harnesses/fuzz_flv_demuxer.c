// Fuzzes libflv's tag demuxer (AAC/MP3/H.264/H.265/AV1/VP9 tag bodies, AMF0 onMetaData script tags).
// Input is a sequence of records: [kind:1][len:2 LE][body:len]; kind % 3 -> FLV tag type 8/9/18, so
// codec state (sequence headers, then frames) carries across tags like a real stream.
#include <stddef.h>
#include <stdint.h>
#include "flv-demuxer.h"
#include "flv-proto.h"

static int on_av(void *param, int codec, const void *data, size_t bytes, uint32_t pts, uint32_t dts, int flags)
{
    volatile unsigned *sink = (volatile unsigned *)param;
    const uint8_t *p = (const uint8_t *)data;
    for (size_t i = 0; i < bytes; i++)
        *sink += p[i];
    *sink += (unsigned)codec + pts + dts + (unsigned)flags;
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static const int types[3] = {FLV_TYPE_AUDIO, FLV_TYPE_VIDEO, FLV_TYPE_SCRIPT};
    if (size < 3)
        return -1;

    volatile unsigned sink = 0;
    flv_demuxer_t *demux = flv_demuxer_create(on_av, (void *)&sink);
    if (!demux)
        return -1;

    uint32_t ts = 0;
    while (size >= 3) {
        int type = types[data[0] % 3];
        size_t len = (size_t)data[1] | ((size_t)data[2] << 8);
        data += 3;
        size -= 3;
        if (len > size)
            len = size;
        if (flv_demuxer_input(demux, type, data, len, ts) != 0)
            break;
        data += len;
        size -= len;
        ts += 40;
    }

    flv_demuxer_destroy(demux);
    return 0;
}
