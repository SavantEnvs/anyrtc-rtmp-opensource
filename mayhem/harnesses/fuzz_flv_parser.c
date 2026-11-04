// Fuzzes libflv's streaming FLV parser (header + tag stream, incl. AVC/HEVC/AAC tag headers).
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "flv-parser.h"

static int on_av(void *param, int codec, const void *data, size_t bytes, uint32_t pts, uint32_t dts, int flags)
{
    volatile unsigned *sink = (volatile unsigned *)param;
    const uint8_t *p = (const uint8_t *)data;
    for (size_t i = 0; i < bytes; i++)
        *sink += p[i];
    *sink += (unsigned)codec + pts + dts + (unsigned)flags;
    return 0;
}

// The parser frees a tag body itself once the tag is complete but leaves parser.body dangling, so
// track the live allocation to release a body left over by input that ends mid-tag.
static void *live_body;

static void *body_alloc(void *param, size_t bytes)
{
    (void)param;
    live_body = malloc(bytes);
    return live_body;
}

static void body_free(void *param, void *ptr)
{
    (void)param;
    free(ptr);
    if (ptr == live_body)
        live_body = NULL;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 9)
        return -1;

    volatile unsigned sink = 0;
    struct flv_parser_t parser;
    memset(&parser, 0, sizeof(parser));
    parser.alloc = body_alloc;
    parser.free = body_free;
    live_body = NULL;

    // Deliver in two pieces (split point taken from the first byte) to exercise the
    // incremental-parse state machine as a network reader would.
    size_t first = 1 + (size_t)data[0] % size;
    if (flv_parser_input(&parser, data, first, on_av, (void *)&sink) == 0 && first < size)
        flv_parser_input(&parser, data + first, size - first, on_av, (void *)&sink);

    free(live_body);
    live_body = NULL;
    return 0;
}
