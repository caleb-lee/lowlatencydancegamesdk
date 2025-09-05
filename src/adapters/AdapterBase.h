#ifndef DANCEPADADAPTERBASE_H
#define DANCEPADADAPTERBASE_H

#include <stdint.h>

struct DancePadAdapter {
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t (*input_converter)(uint8_t[], int);
};

#endif