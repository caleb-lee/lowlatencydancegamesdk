#ifndef DANCEPADADAPTERBASE_H
#define DANCEPADADAPTERBASE_H

#include <stdint.h>
#include <stdbool.h>
#include <libusb.h>

struct DancePadAdapter {
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t (*input_converter)(uint8_t[], int);
    bool (*is_p2)(libusb_device_handle*, uint8_t, uint8_t); // Optional; please set to `NULL` if pad has no logic for P1/P2
};

typedef enum {
    // Cardinal directions
    DancePadAdapterInputLeft  = 1 << 3,
    DancePadAdapterInputDown  = 1 << 7,
    DancePadAdapterInputUp    = 1 << 1,
    DancePadAdapterInputRight = 1 << 5,

    // Diagonals and center
    DancePadAdapterInputUpLeft    = 1 << 0,
    DancePadAdapterInputUpRight   = 1 << 2,
    DancePadAdapterInputCenter    = 1 << 4,
    DancePadAdapterInputDownLeft  = 1 << 6,
    DancePadAdapterInputDownRight = 1 << 8,

    // Other possible inputs
    DancePadAdapterInputStart  = 1 << 9,
    DancePadAdapterInputSelect = 1 << 10,

    // Useful constants
    DancePadAdapterInputNone = 0,
    DancePadAdapterInputSMXDLLMask = 0b0000000111111111,
} DancePadAdapterInputEnum; typedef uint16_t DancePadAdapterInput;

#endif