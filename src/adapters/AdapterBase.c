#include "AdapterBase.h"
#include "SMXStage/SMXStageAdapter.h"
#include "FoamPad/FoamPadAdapter.h"

extern struct DancePadAdapter dance_pad_adapter_for(uint16_t vendor_id, uint16_t product_id) {
    static bool adapters_initialized = false;
    static struct DancePadAdapter adapters[2];
    if (!adapters_initialized) {
        adapters[0] = default_smx_adapter();
        adapters[1] = default_foam_pad_adapter();
        adapters_initialized = true;
    }
    
    for (int i = 0; i < sizeof(adapters); i++) {
        if (vendor_id == adapters[i].vendor_id && product_id == adapters[i].product_id) {
            return adapters[i];
        }
    }

    struct DancePadAdapter invalidAdapter;
    invalidAdapter.is_valid = false;
    return invalidAdapter;
}

extern bool dance_pad_is_pid_vid_valid_pad(uint16_t vendor_id, uint16_t product_id) {
    return dance_pad_adapter_for(vendor_id, product_id).is_valid;
}