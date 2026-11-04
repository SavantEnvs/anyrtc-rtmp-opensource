// Known-answer probe for the oracle (mayhem/test.sh). Built with the project's normal flags,
// run as a dynamically linked binary; test.sh compares its output line by line with kat.expected.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "amf0.h"
#include "flv-parser.h"
#include "flv-proto.h"
#include "neaacdec.h"
#include "rtmp-client.h"
#include "kat_data.h"

static int on_flv(void *param, int codec, const void *data, size_t bytes, uint32_t pts, uint32_t dts, int flags)
{
    (void)param; (void)data;
    printf("flv codec=%d bytes=%zu pts=%u dts=%u flags=%d\n", codec, bytes, pts, dts, flags);
    return 0;
}

static int on_rtmp(const char *kind, const void *data, size_t bytes, uint32_t ts)
{
    printf("rtmp %s bytes=%zu ts=%u first=0x%02x\n", kind, bytes, ts, ((const unsigned char *)data)[0]);
    return 0;
}
static int on_audio(void *p, const void *d, size_t n, uint32_t ts) { (void)p; return on_rtmp("audio", d, n, ts); }
static int on_video(void *p, const void *d, size_t n, uint32_t ts) { (void)p; return on_rtmp("video", d, n, ts); }
static int on_script(void *p, const void *d, size_t n, uint32_t ts) { (void)p; return on_rtmp("script", d, n, ts); }
static int on_send(void *p, const void *h, size_t hl, const void *d, size_t n) { (void)p; (void)h; (void)d; return (int)(hl + n); }

static void kat_amf(void)
{
    // AMF0: "_result", 1.0, { code: "NetConnection.Connect.Success", clientid: 7 }
    static const unsigned char msg[] = {
        0x02, 0x00, 0x07, '_', 'r', 'e', 's', 'u', 'l', 't',
        0x00, 0x3f, 0xf0, 0, 0, 0, 0, 0, 0,
        0x03, 0x00, 0x04, 'c', 'o', 'd', 'e', 0x02, 0x00, 0x1d,
        'N', 'e', 't', 'C', 'o', 'n', 'n', 'e', 'c', 't', 'i', 'o', 'n', '.', 'C', 'o', 'n', 'n', 'e', 'c', 't', '.',
        'S', 'u', 'c', 'c', 'e', 's', 's',
        0x00, 0x08, 'c', 'l', 'i', 'e', 'n', 't', 'i', 'd', 0x00, 0x40, 0x1c, 0, 0, 0, 0, 0, 0,
        0x00, 0x00, 0x09,
    };
    char name[32] = "", code[64] = "";
    double txn = 0, cid = 0;
    struct amf_object_item_t props[] = {
        {AMF_STRING, "code", code, sizeof(code)},
        {AMF_NUMBER, "clientid", &cid, sizeof(cid)},
    };
    struct amf_object_item_t items[] = {
        {AMF_STRING, "command", name, sizeof(name)},
        {AMF_NUMBER, "txn", &txn, sizeof(txn)},
        {AMF_OBJECT, "props", props, 2},
    };
    const unsigned char *end = amf_read_items(msg, msg + sizeof(msg), items, 3);
    printf("amf consumed=%ld command=%s txn=%g code=%s clientid=%g\n",
           end ? (long)(end - msg) : -1L, name, txn, code, cid);
}

static void kat_flv(void)
{
    // FLV header + one MP3 audio tag (4 payload bytes, dts 40 ms) + trailing PreviousTagSize.
    static const unsigned char flv[] = {
        'F', 'L', 'V', 1, 4, 0, 0, 0, 9, 0, 0, 0, 0,
        8, 0, 0, 5, 0, 0, 40, 0, 0, 0, 0,
        0x2f, 0x11, 0x22, 0x33, 0x44,
        0, 0, 0, 16,
    };
    struct flv_parser_t parser;
    memset(&parser, 0, sizeof(parser));
    int r = flv_parser_input(&parser, flv, sizeof(flv), on_flv, NULL);
    printf("flv result=%d\n", r);
}

static void kat_faad(void)
{
    static const unsigned char asc[2] = {0x12, 0x10}; // AAC-LC, 44.1 kHz, stereo
    unsigned long sr = 0;
    unsigned char ch = 0;
    NeAACDecHandle h = NeAACDecOpen();
    long r = NeAACDecInit2(h, (unsigned char *)asc, 2, &sr, &ch);
    printf("faad init=%ld samplerate=%lu channels=%u\n", r, sr, ch);
    const unsigned char *frames[3] = {kat_aac_frame0, kat_aac_frame1, kat_aac_frame2};
    const size_t sizes[3] = {sizeof(kat_aac_frame0), sizeof(kat_aac_frame1), sizeof(kat_aac_frame2)};
    for (int f = 0; f < 3; f++) {
        NeAACDecFrameInfo info;
        memset(&info, 0, sizeof(info));
        short *pcm = (short *)NeAACDecDecode(h, &info, (unsigned char *)frames[f], sizes[f]);
        int peak = 0;
        for (unsigned long i = 0; pcm && i < info.samples; i++) {
            int v = pcm[i] < 0 ? -pcm[i] : pcm[i];
            if (v > peak) peak = v;
        }
        printf("faad frame=%d size=%zu consumed=%lu samples=%lu channels=%u error=%u audible=%d\n",
               f, sizes[f], info.bytesconsumed, info.samples, info.channels, info.error, peak > 1000);
    }
    NeAACDecClose(h);
}

static void kat_rtmp(void)
{
    static unsigned char hs[1 + 2 * 1536] = {3};
    struct rtmp_client_handler_t handler;
    memset(&handler, 0, sizeof(handler));
    handler.send = on_send;
    handler.onaudio = on_audio;
    handler.onvideo = on_video;
    handler.onscript = on_script;
    rtmp_client_t *c = rtmp_client_create("live", "stream", "rtmp://127.0.0.1/live", NULL, &handler);
    int a = rtmp_client_start(c, 1);
    int b = rtmp_client_input(c, hs, sizeof(hs));
    int d = rtmp_client_input(c, kat_rtmp_stream, sizeof(kat_rtmp_stream));
    printf("rtmp start=%d handshake=%d input=%d\n", a, b, d);
    rtmp_client_destroy(c);
}

int main(void)
{
    kat_amf();
    kat_flv();
    kat_faad();
    kat_rtmp();
    return 0;
}
