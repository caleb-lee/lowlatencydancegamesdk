#include <windows.h>
#include <mutex>
#include <cstring>

#include "SMX.h"
#include "../../include/lowlatencydancegamesdk.h"
extern "C" {
    #include "../adapters/AdapterBase.h"
}

using namespace std;

// Global state
static SMXUpdateCallback* g_UpdateCallback;
static void* g_pUserData;
static mutex g_stateMutex;

// Cached state
static SMXInfo g_info[LowLatencyDanceGameSDK::MAX_PLAYERS];

// Input callback function that bridges from LLDGSDK to SMX API
static void OnInputReceived(LowLatencyDanceGameSDK::Player player, uint16_t button_state, void* user_data)
{
    int pad = static_cast<int>(player);
    
    // Call the SMX update callback
    if (g_UpdateCallback)
    {
        g_UpdateCallback(pad, SMXUpdateCallback_Updated, g_pUserData);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch(ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

// Public API Implementation
SMX_API void SMX_Start(SMXUpdateCallback callback, void *pUser)
{
    g_UpdateCallback = callback;
    g_pUserData = pUser;

    // Initialize the SDK with our input callback
    auto& sdk = LowLatencyDanceGameSDK::getInstance();
    if (!sdk.initialize(OnInputReceived, nullptr))
        return;
    
    // Set up initial state
    {
        lock_guard<mutex> lock(g_stateMutex);
        memset(g_info, 0, sizeof(g_info));
        
        for (int pad = 0; pad < LowLatencyDanceGameSDK::MAX_PLAYERS; pad++)
        {
            LowLatencyDanceGameSDK::Player player = static_cast<LowLatencyDanceGameSDK::Player>(pad);
            if (sdk.isPlayerConnected(player))
            {
                g_info[pad].m_bConnected = true;
                sprintf_s(g_info[pad].m_Serial, sizeof(g_info[pad].m_Serial), "LLDGSDK-P%d", pad + 1);
                g_info[pad].m_iFirmwareVersion = 0x0500; // Fake version 5.0
            }
            else
            {
                g_info[pad].m_bConnected = false;
            }
        }
    }

    // Initial callback for all connected pads
    if (g_UpdateCallback)
    {
        for (int pad = 0; pad < LowLatencyDanceGameSDK::MAX_PLAYERS; pad++)
        {
            LowLatencyDanceGameSDK::Player player = static_cast<LowLatencyDanceGameSDK::Player>(pad);
            if (sdk.isPlayerConnected(player))
            {
                g_UpdateCallback(pad, SMXUpdateCallback_Updated, g_pUserData);
            }
        }
    }
}

SMX_API void SMX_Stop()
{
    // Shutdown the SDK
    auto& sdk = LowLatencyDanceGameSDK::getInstance();
    sdk.shutdown();
    
    g_UpdateCallback = nullptr;
    g_pUserData = nullptr;
}

SMX_API void SMX_SetLogCallback(SMXLogCallback callback)
{
    // No-op for now
}

SMX_API void SMX_GetInfo(int pad, SMXInfo *info)
{
    if (!info)
        return;

    if (pad < 0 || pad >= LowLatencyDanceGameSDK::MAX_PLAYERS)
    {
        memset(info, 0, sizeof(SMXInfo));
        return;
    }

    lock_guard<mutex> lock(g_stateMutex);
    
    // Update connection status from SDK
    auto& sdk = LowLatencyDanceGameSDK::getInstance();
    LowLatencyDanceGameSDK::Player player = static_cast<LowLatencyDanceGameSDK::Player>(pad);
    g_info[pad].m_bConnected = sdk.isPlayerConnected(player);
    
    *info = g_info[pad];
}

SMX_API uint16_t SMX_GetInputState(int pad)
{
    if (pad < 0 || pad >= LowLatencyDanceGameSDK::MAX_PLAYERS) {
        return 0;
    }
        
    auto& sdk = LowLatencyDanceGameSDK::getInstance();
    LowLatencyDanceGameSDK::Player player = static_cast<LowLatencyDanceGameSDK::Player>(pad);
    return sdk.getPlayerButtonState(player) & DancePadAdapterInputSMXDLLMask; // mask to expected bits for SMX.dll integrations
}

// Light functions - all no-ops
SMX_API void SMX_SetLights(const char lightData[864])
{
    // No-op
}

SMX_API void SMX_SetLights2(const char *lightData, int lightDataSize)
{
    // No-op
}

SMX_API void SMX_ReenableAutoLights()
{
    // No-op
}

// Config functions - minimal implementation
SMX_API bool SMX_GetConfig(int pad, SMXConfig *config)
{
    if (!config || pad < 0 || pad >= LowLatencyDanceGameSDK::MAX_PLAYERS)
        return false;

    // Return default config
    memset(config, 0, sizeof(SMXConfig));
    config->masterVersion = 0x05;
    config->configVersion = 0x05;
    
    return g_info[pad].m_bConnected;
}

SMX_API void SMX_SetConfig(int pad, const SMXConfig *config)
{
    // No-op
}

SMX_API void SMX_FactoryReset(int pad)
{
    // No-op
}

SMX_API void SMX_ForceRecalibration(int pad)
{
    // No-op
}

// Test mode functions - no-ops
SMX_API void SMX_SetTestMode(int pad, SensorTestMode mode)
{
    // No-op
}

SMX_API bool SMX_GetTestData(int pad, SMXSensorTestModeData *data)
{
    if (!data || pad != 0)
        return false;

    memset(data, 0, sizeof(SMXSensorTestModeData));
    return false;
}

SMX_API void SMX_SetPanelTestMode(PanelTestMode mode)
{
    // No-op
}

SMX_API const char *SMX_Version()
{
    return "LLDGSDK-1.0.0";
}