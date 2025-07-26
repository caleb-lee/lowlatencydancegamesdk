#ifndef LOWLATENCYDANCEGAMESDK_H
#define LOWLATENCYDANCEGAMESDK_H

#include <cstdint>

class LowLatencyDanceGameSDK {
public:
    // Constructor - can fail (check is_valid())
    LowLatencyDanceGameSDK(uint16_t vid, uint16_t pid);
    ~LowLatencyDanceGameSDK();
    
    // Check if constructor succeeded
    bool is_valid() const;
    
    // Request data - returns number of bytes read, -1 on error
    int read_data(uint8_t* buffer, int count);

private:
    // TODO: Add implementation details
};

#endif