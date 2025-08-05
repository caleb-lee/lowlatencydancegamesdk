#ifndef LOWLATENCYDANCEGAMESDK_H
#define LOWLATENCYDANCEGAMESDK_H

#include <cstdint>

class LowLatencyDanceGameSDK {
public:
    static constexpr int MAX_PADS = 2;
    
    // Static device management - abstracts all USB details
    static bool initialize();    // Sets up device discovery system
    static void shutdown();
    static LowLatencyDanceGameSDK** discover_devices(); // Returns array[MAX_PADS]: [P1, P2], NULL for missing pads

    // Instance methods
    bool is_valid() const;
    int get_player_number() const; // 0 or 1 (P1 or P2), matches array index
    int read_data(uint8_t* buffer, int expectedCount);
    
    ~LowLatencyDanceGameSDK();

private:
    struct DanceGameDevice;
    DanceGameDevice *m_device;
    bool m_isValid;
    int m_playerNumber; // 0 or 1
    
    // Private constructor - only static methods create instances
    LowLatencyDanceGameSDK();
    
    // Helper methods for device setup
    static bool setup_device_from_handle(void* handle, DanceGameDevice* device);
    static int get_player_number_from_device(void* handle, uint8_t interrupt_in_endpoint, uint8_t interrupt_out_endpoint);
};

#endif