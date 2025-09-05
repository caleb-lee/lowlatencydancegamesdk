#ifndef DANCEPADADAPTERBASE_H
#define DANCEPADADAPTERBASE_H

#include <stdint.h>
#include <libusb.h>

typedef enum {
    DancePadAdapterPlayerUnknown = -1,
    DancePadAdapterPlayer1       = 0,
    DancePadAdapterPlayer2,
} DancePadAdapterPlayerEnum; typedef int DancePadAdapterPlayer;

struct DancePadAdapter {
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t (*input_converter)(uint8_t[], int);
    DancePadAdapterPlayer (*get_player)(libusb_device_handle*, uint8_t, uint8_t);
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