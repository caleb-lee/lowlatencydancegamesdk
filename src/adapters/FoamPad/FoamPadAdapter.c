#include "FoamPadAdapter.h"

uint16_t input_response_converter(uint8_t data[], int length) {
    if (length < 7) {
        return 0;
    }

    uint8_t arrow_buttons  = data[5];
    uint8_t action_buttons = data[6];
    DancePadAdapterInput result = DancePadAdapterInputNone;

    // Map arrows
    if (arrow_buttons & 0x40) result |= DancePadAdapterInputLeft;
    if (arrow_buttons & 0x20) result |= DancePadAdapterInputDown;
    if (arrow_buttons & 0x10) result |= DancePadAdapterInputUp;
    if (arrow_buttons & 0x80) result |= DancePadAdapterInputRight;

    // Map diagonals
    if (action_buttons & 0x04) result |= DancePadAdapterInputUpLeft;
    if (action_buttons & 0x08) result |= DancePadAdapterInputUpRight;
    if (action_buttons & 0x01) result |= DancePadAdapterInputDownLeft;
    if (action_buttons & 0x02) result |= DancePadAdapterInputDownRight;

    // Start and Select
    if (action_buttons & 0x20) result |= DancePadAdapterInputStart;
    if (action_buttons & 0x10) result |= DancePadAdapterInputSelect;

    return (uint16_t)result;
}

extern struct DancePadAdapter default_foam_pad_adapter() {
    struct DancePadAdapter adapter;

    adapter.vendor_id = 0x2341; //TODO: Update
    adapter.product_id = 0x8037; //TODO: Update
    adapter.input_converter = input_response_converter;
    adapter.is_p2 = NULL;

    return adapter;
}