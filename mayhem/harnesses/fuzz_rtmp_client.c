// Fuzzes the RTMP client receive path (chunk stream reassembly, protocol/user control messages,
// AMF0/AMF3 command handling, audio/video/data delivery). A fixed, valid S0+S1+S2 handshake is
// delivered first; the fuzzer input is the server->client chunk stream that follows.
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "rtmp-client.h"

#define HANDSHAKE_SIZE 1536

static int on_send(void *param, const void *header, size_t len, const void *payload, size_t bytes)
{
    (void)param; (void)header; (void)payload;
    return (int)(len + bytes);
}

static int touch(void *param, const void *data, size_t bytes, uint32_t timestamp)
{
    volatile unsigned *sink = (volatile unsigned *)param;
    const uint8_t *p = (const uint8_t *)data;
    for (size_t i = 0; i < bytes; i++)
        *sink += p[i];
    *sink += timestamp;
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static uint8_t handshake[1 + 2 * HANDSHAKE_SIZE]; // S0 (version 3) + S1 + S2
    if (size < 2)
        return -1;
    handshake[0] = 3;

    volatile unsigned sink = 0;
    struct rtmp_client_handler_t handler;
    memset(&handler, 0, sizeof(handler));
    handler.send = on_send;
    handler.onvideo = touch;
    handler.onaudio = touch;
    handler.onscript = touch;

    rtmp_client_t *c = rtmp_client_create("live", "stream", "rtmp://127.0.0.1/live", (void *)&sink, &handler);
    if (!c)
        return -1;

    if (rtmp_client_start(c, 1) == 0 && rtmp_client_input(c, handshake, sizeof(handshake)) == 0) {
        // Feed in pieces (size from the first byte) so partial headers/payloads are exercised.
        size_t step = 1 + (size_t)data[0];
        data++;
        size--;
        while (size > 0) {
            size_t n = size < step ? size : step;
            if (rtmp_client_input(c, data, n) != 0)
                break;
            data += n;
            size -= n;
        }
    }

    rtmp_client_destroy(c);
    return 0;
}
