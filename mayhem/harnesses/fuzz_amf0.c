// Fuzzes libflv's AMF0 reader: scalar readers plus amf_read_items with a nested object table of
// the shape used for connect/onStatus/onMetaData handling.
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "amf0.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 2)
        return -1;

    const uint8_t *end = data + size;
    uint8_t sel = data[0];
    const uint8_t *p = data + 1;

    char name[64], code[64], desc[128];
    double num = 0, num2 = 0, dur = 0;
    uint8_t flag = 0, flag2 = 0;
    double date = 0;
    int16_t tz = 0;
    char str[256];
    memset(name, 0, sizeof(name));
    memset(code, 0, sizeof(code));
    memset(desc, 0, sizeof(desc));

    switch (sel % 6) {
    case 0:
        AMFReadString(p, end, 0, str, sizeof(str));
        break;
    case 1:
        AMFReadString(p, end, 1, str, sizeof(str));
        break;
    case 2:
        AMFReadDouble(p, end, &num);
        AMFReadBoolean(p, end, &flag);
        AMFReadDate(p, end, &date, &tz);
        break;
    case 3: {
        struct amf_object_item_t status[] = {
            {AMF_STRING, "code", code, sizeof(code)},
            {AMF_STRING, "description", desc, sizeof(desc)},
            {AMF_NUMBER, "clientid", &num2, sizeof(num2)},
        };
        struct amf_object_item_t items[] = {
            {AMF_STRING, "command", name, sizeof(name)},
            {AMF_NUMBER, "transaction", &num, sizeof(num)},
            {AMF_OBJECT, "properties", status, sizeof(status) / sizeof(status[0])},
            {AMF_BOOLEAN, "flag", &flag2, sizeof(flag2)},
        };
        amf_read_items(p, end, items, sizeof(items) / sizeof(items[0]));
        break;
    }
    case 4: {
        struct amf_object_item_t meta[] = {
            {AMF_NUMBER, "duration", &dur, sizeof(dur)},
            {AMF_NUMBER, "width", &num, sizeof(num)},
            {AMF_NUMBER, "height", &num2, sizeof(num2)},
            {AMF_STRING, "encoder", desc, sizeof(desc)},
            {AMF_BOOLEAN, "stereo", &flag, sizeof(flag)},
        };
        struct amf_object_item_t items[] = {
            {AMF_STRING, "name", name, sizeof(name)},
            {AMF_ECMA_ARRAY, "metadata", meta, sizeof(meta) / sizeof(meta[0])},
        };
        amf_read_items(p, end, items, sizeof(items) / sizeof(items[0]));
        break;
    }
    default: {
        struct amf_object_item_t items[] = {
            {AMF_NUMBER, "a", &num, sizeof(num)},
            {AMF_NUMBER, "b", &num2, sizeof(num2)},
            {AMF_STRING, "c", str, sizeof(str)},
        };
        amf_read_items(p, end, items, sizeof(items) / sizeof(items[0]));
        break;
    }
    }
    return 0;
}
