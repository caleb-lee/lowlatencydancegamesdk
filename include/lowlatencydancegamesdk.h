#ifndef LOWLATENCYDANCEGAMESDK_H
#define LOWLATENCYDANCEGAMESDK_H

#include <cstdint>

class LowLatencyDanceGameSDK {
public:
    // Constructor - can fail (check is_valid())
    LowLatencyDanceGameSDK(uint16_t vendor_id, uint16_t product_id);
    ~LowLatencyDanceGameSDK();
    
    // Check if constructor succeeded
    bool is_valid() const;
    
    // Request data - returns number of bytes read, < 0 on error
    int read_data(uint8_t* buffer, int expectedCount);

private:
    struct DanceGameDevice;
    DanceGameDevice *m_device;

    bool m_isValid;
};

#endif