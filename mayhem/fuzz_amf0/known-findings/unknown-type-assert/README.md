# amf_read_item aborts on an unknown AMF0 type marker

`input.bin` selects the ECMA-array case (`04`); the array holds a property whose value type byte is `09`
(the object-end marker), which `amf_read_item` does not handle, so `libflv/source/amf0.c:331`
(`default: assert(0)`) aborts. Any peer-supplied AMF0 object with an unrecognised type byte reaches it,
from RTMP command/data messages and FLV `onMetaData`.

Impact: remote abort (DoS) of any embedder built with asserts enabled. Fix: `return NULL` instead of `assert(0)`.
