#include "SMXStageAdapter.h"

uint16_t smx_input_response_converter(uint8_t data[], int length) {
    if (length < 3) {
        return 0;
    }

    return ((data[2] & 0xFF) << 8) | ((data[1] & 0xFF) << 0);
}

extern struct DancePadAdapter default_smx_adapter() {
    struct DancePadAdapter adapter;

    adapter.vendor_id = 0x2341;
    adapter.product_id = 0x8037;
    adapter.input_converter = smx_input_response_converter;

    return adapter;
}