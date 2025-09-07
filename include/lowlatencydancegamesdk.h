#ifndef LOWLATENCYDANCEGAMESDK_H
#define LOWLATENCYDANCEGAMESDK_H

#include <cstdint>
#include <memory>

class LowLatencyDanceGameSDK {
public:
    enum class Player {
        P1 = 0,
        P2,
    };
    
    using InputCallback = void(*)(Player player, uint16_t button_state, void* user_data);
    
    static constexpr int MAX_PLAYERS = 2;
    static LowLatencyDanceGameSDK& getInstance();

    static bool isPadCompatible(uint16_t vendor_id, uint16_t product_id);
    
    bool initialize(InputCallback callback, void* user_data);
    void shutdown();
    
    bool isPlayerConnected(Player player);
    uint16_t getPlayerButtonState(Player player);
    
private:
    LowLatencyDanceGameSDK();
    ~LowLatencyDanceGameSDK();
    
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    
    LowLatencyDanceGameSDK(const LowLatencyDanceGameSDK&) = delete;
    LowLatencyDanceGameSDK& operator=(const LowLatencyDanceGameSDK&) = delete;
};

#endif