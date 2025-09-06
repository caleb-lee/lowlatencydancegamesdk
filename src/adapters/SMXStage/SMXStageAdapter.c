#include "SMXStageAdapter.h"

uint16_t input_converter(uint8_t data[], int length) {
    if (length < 3) {
        return 0;
    }

    return ((data[2] & 0xFF) << 8) | ((data[1] & 0xFF) << 0);
}

DancePadAdapterPlayer get_player(libusb_device_handle *handle, uint8_t interrupt_in_endpoint, uint8_t interrupt_out_endpoint)
{
    if (interrupt_out_endpoint == 0)
    {
        return DancePadAdapterPlayerUnknown;
    }

    const unsigned char data[] = {5, 0x80, 0};
    int bytes_sent = 0;
    int result = libusb_interrupt_transfer(handle, interrupt_out_endpoint,
                                           (unsigned char *)data, sizeof(data),
                                           &bytes_sent, 1000);
    if (result < 0 || bytes_sent < sizeof(data))
    {
        return DancePadAdapterPlayerUnknown;
    }

    unsigned char buf[65];
    int bytes_read = 0;
    result = libusb_interrupt_transfer(handle, interrupt_in_endpoint,
                                       buf, sizeof(buf),
                                       &bytes_read, 1000);

    if (result < 0 || bytes_read < 4)
    {
        return DancePadAdapterPlayerUnknown;
    }

    return (buf[3] == '1') ? DancePadAdapterPlayer2 : DancePadAdapterPlayer1;
}

extern struct DancePadAdapter default_smx_adapter() {
    struct DancePadAdapter adapter;

    adapter.vendor_id = 0x2341;
    adapter.product_id = 0x8037;
    adapter.input_converter = input_converter;
    adapter.get_player = get_player;
    adapter.is_valid = true;

    return adapter;
}