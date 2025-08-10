#ifndef LOWLATENCYDANCEGAMESDK_H
#define LOWLATENCYDANCEGAMESDK_H

#include <cstdint>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>

class LowLatencyDanceGameSDK {
public:
    enum class Player {
        P1 = 0,
        P2,
        PLAYER_COUNT
    };
    
    using InputCallback = std::function<void(Player player, uint16_t button_state)>;
    
    static constexpr int MAX_PLAYERS = static_cast<int>(Player::PLAYER_COUNT);
    static LowLatencyDanceGameSDK& getInstance();
    
    bool initialize(InputCallback callback);
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