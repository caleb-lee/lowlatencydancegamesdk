#include "lowlatencydancegamesdk.h"
#include <libusb.h>
#include <thread>
#include <atomic>
#include <cassert>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

extern "C" {
    #include "adapters/AdapterBase.h"
}

// Global libusb context for device management
static libusb_context* g_libusb_ctx = nullptr;

// Set current thread to high priority for low latency
static void setThreadHighPriority() {
#ifdef _WIN32
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#else // Linux and Mac
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
#endif
}

// Returns true if device_a should come before device_b in USB ordering
static bool compareUSBLocation(libusb_device* device_a, libusb_device* device_b) {
    uint8_t bus_a = libusb_get_bus_number(device_a);
    uint8_t bus_b = libusb_get_bus_number(device_b);
    if (bus_a != bus_b) return bus_a < bus_b;
    
    uint8_t path_a[8], path_b[8];
    int len_a = libusb_get_port_numbers(device_a, path_a, sizeof(path_a));
    int len_b = libusb_get_port_numbers(device_b, path_b, sizeof(path_b));
    
    int min_len = (len_a < len_b) ? len_a : len_b;
    for (int i = 0; i < min_len; i++) {
        if (path_a[i] != path_b[i]) return path_a[i] < path_b[i];
    }
    return len_a < len_b;
}

struct DeviceState {
    libusb_device_handle* handle = nullptr;
    libusb_device* device = nullptr;
    libusb_transfer* transfer = nullptr;
    uint8_t interrupt_in_endpoint = 0;
    uint8_t interrupt_out_endpoint = 0;
    uint8_t hid_interface = 0;
    bool connected = false;
    uint16_t nonatomic_last_button_state = 0;
    std::atomic<uint16_t> last_button_state{0};
    unsigned char buffer[65];
    DancePadAdapterPlayer player;
    struct DancePadAdapter adapter;
    void* impl;
};

struct LowLatencyDanceGameSDK::Impl {
    DeviceState* devices[MAX_PLAYERS] = {nullptr};
    InputCallback inputCallback;
    bool initialized = false;
    std::atomic<bool> shutdown{false};
    std::unique_ptr<std::thread> usbThread;

    static void LIBUSB_CALL transferCallback(libusb_transfer* transfer) {
        DeviceState* device = static_cast<DeviceState*>(transfer->user_data);
        static_cast<Impl*>(device->impl)->handleTransferComplete(transfer);
    }

    void handleTransferComplete(libusb_transfer* transfer) {
        DeviceState *device = static_cast<DeviceState *>(transfer->user_data);

        // Assert that we are within bounds of the `Player` enum before proceeding
        assert(device->player >= 0 && device->player < MAX_PLAYERS);

        // Got an error; disconnect device and return without submitting another transfer
        if (transfer->status != LIBUSB_TRANSFER_COMPLETED && transfer->status != LIBUSB_TRANSFER_TIMED_OUT) {
            device->connected = false;
            return;
        }

        // Parse out the input
        uint16_t new_state = device->adapter.input_converter(device->buffer, transfer->actual_length);

        // If the input state is different from the last input state we received, call the callback
        if (new_state != device->nonatomic_last_button_state) {
            device->last_button_state = new_state;
            if (inputCallback) {
                inputCallback(static_cast<Player>(device->player), new_state);
            }
            device->nonatomic_last_button_state = new_state;
        }

        if (shutdown) {
            return;
        }

        // If we reach this point, immediately submit a new transfer
        libusb_submit_transfer(transfer);
    }

    bool setupDevice(libusb_device_handle* handle, DeviceState* device) {
        struct libusb_config_descriptor *config;
        libusb_get_active_config_descriptor(libusb_get_device(handle), &config);
        
        int hid_interface = -1;
        int hid_interface_index = -1;
        for (int i = 0; i < config->bNumInterfaces; i++) {
            const struct libusb_interface_descriptor *intf = &config->interface[i].altsetting[0];
            if (intf->bInterfaceClass == 3) {
                hid_interface = intf->bInterfaceNumber;
                hid_interface_index = i;
                break;
            }
        }
        
        if (hid_interface == -1) {
            libusb_free_config_descriptor(config);
            return false;
        }
        
        if (libusb_kernel_driver_active(handle, hid_interface) == 1) {
            if (libusb_detach_kernel_driver(handle, hid_interface) != 0) {
                libusb_free_config_descriptor(config);
                return false;
            }
        }
        
        if (libusb_claim_interface(handle, hid_interface) < 0) {
            libusb_free_config_descriptor(config);
            return false;
        }
        
        uint8_t interrupt_in_endpoint = 0;
        uint8_t interrupt_out_endpoint = 0;
        
        for (int i = 0; i < config->interface[hid_interface_index].altsetting[0].bNumEndpoints; i++) {
            const struct libusb_endpoint_descriptor *ep = &config->interface[hid_interface_index].altsetting[0].endpoint[i];
            
            bool is_interrupt = (ep->bmAttributes & 0x03) == 0x03;
            bool is_input = (ep->bEndpointAddress & 0x80) != 0;
            bool is_output = (ep->bEndpointAddress & 0x80) == 0;
            
            if (is_interrupt && is_input && interrupt_in_endpoint == 0) {
                interrupt_in_endpoint = ep->bEndpointAddress;
            }
            if (is_interrupt && is_output && interrupt_out_endpoint == 0) {
                interrupt_out_endpoint = ep->bEndpointAddress;
            }
        }
        
        libusb_free_config_descriptor(config);
        
        if (interrupt_in_endpoint == 0) {
            libusb_release_interface(handle, hid_interface);
            return false;
        }
        
        device->handle = handle;
        device->interrupt_in_endpoint = interrupt_in_endpoint;
        device->interrupt_out_endpoint = interrupt_out_endpoint;
        device->hid_interface = hid_interface;
        device->player = device->adapter.get_player(handle, interrupt_in_endpoint, interrupt_out_endpoint);
        device->connected = true;
        device->impl = this;
        
        device->transfer = libusb_alloc_transfer(0);
        if (!device->transfer) {
            libusb_release_interface(handle, hid_interface);
            return false;
        }
        
        libusb_fill_interrupt_transfer(
            device->transfer,
            handle,
            interrupt_in_endpoint,
            device->buffer,
            sizeof(device->buffer),
            transferCallback,
            device,
            1000
        );
        
        if (libusb_submit_transfer(device->transfer) < 0) {
            libusb_free_transfer(device->transfer);
            device->transfer = nullptr;
            libusb_release_interface(handle, hid_interface);
            return false;
        }
        
        return true;
    }

    bool discoverDevices() {
        libusb_device **device_list;
        ssize_t device_count = libusb_get_device_list(g_libusb_ctx, &device_list);
        
        if (device_count < 0) {
            return false;
        }
        
        int found_devices = 0;
        
        for (ssize_t i = 0; i < device_count && found_devices < MAX_PLAYERS; i++) {
            struct libusb_device_descriptor desc;
            if (libusb_get_device_descriptor(device_list[i], &desc) < 0) {
                continue;
            }
            
            struct DancePadAdapter adapter = dance_pad_adapter_for(desc.idVendor, desc.idProduct);
            if (!adapter.is_valid) {
                continue;
            }
            
            libusb_device_handle *handle;
            if (libusb_open(device_list[i], &handle) < 0) {
                continue;
            }
            
            DeviceState* device_state = new DeviceState();
            device_state->adapter = adapter;
            device_state->device = device_list[i];
            
            if (!setupDevice(handle, device_state)) {
                delete device_state;
                libusb_close(handle);
                continue;
            }
            
            // Just place devices in order found - will sort later if needed
            if (found_devices < MAX_PLAYERS) {
                devices[found_devices] = device_state;
                found_devices++;
            } else {
                delete device_state;
                libusb_close(handle);
            }
        }
        
        // Handle single device case first
        if (found_devices == 1) {
            DeviceState* single_device = devices[0];
            if (single_device->player == DancePadAdapterPlayer2) {
                // Move P2 preference to correct slot
                devices[1] = single_device;
                devices[0] = nullptr;
            }
            // If P1 or Unknown, leave at slot 0
        } else if (devices[0] && devices[1]) {
            // Sort devices: if either is unknown, use USB port order; otherwise use preferences
            bool has_unknown = (devices[0]->player == DancePadAdapterPlayerUnknown || 
                               devices[1]->player == DancePadAdapterPlayerUnknown);
            
            if (has_unknown) {
                // Use USB port ordering
                if (!compareUSBLocation(devices[0]->device, devices[1]->device)) {
                    DeviceState* temp = devices[0];
                    devices[0] = devices[1];
                    devices[1] = temp;
                }
            } else {
                // Both have preferences - honor them with conflict resolution
                if (devices[0]->player == DancePadAdapterPlayer2 && devices[1]->player == DancePadAdapterPlayer1) {
                    DeviceState* temp = devices[0];
                    devices[0] = devices[1];
                    devices[1] = temp;
                }
                // If both want same slot, first found wins (no swap needed)
            }
        }
        
        // Assign final player values based on array position
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (devices[i]) {
                devices[i]->player = static_cast<DancePadAdapterPlayer>(i);
                devices[i]->device = nullptr;  // Invalidate after use
            }
        }
        
        libusb_free_device_list(device_list, 1);
        return found_devices > 0;
    }

    void cleanupDevices() {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (devices[i]) {
                if (devices[i]->transfer) {
                    libusb_free_transfer(devices[i]->transfer);
                }
                if (devices[i]->handle) {
                    libusb_release_interface(devices[i]->handle, devices[i]->hid_interface);
                    libusb_close(devices[i]->handle);
                }
                delete devices[i];
                devices[i] = nullptr;
            }
        }
    }

    void usbEventLoop() {
        setThreadHighPriority();
        int completed = 0;
        while (!shutdown) {
            libusb_handle_events_completed(g_libusb_ctx, &completed);
        }
    }
};

// Public API implementation
LowLatencyDanceGameSDK& LowLatencyDanceGameSDK::getInstance() {
    static LowLatencyDanceGameSDK instance;
    return instance;
}

LowLatencyDanceGameSDK::LowLatencyDanceGameSDK() : pImpl(std::make_unique<Impl>()) {
}

LowLatencyDanceGameSDK::~LowLatencyDanceGameSDK() {
    if (pImpl) {
        shutdown();
    }
}

bool LowLatencyDanceGameSDK::initialize(InputCallback callback) {
    if (pImpl->initialized) {
        return true;
    }
    
    pImpl->inputCallback = callback;
    pImpl->shutdown = false;
    
    if (g_libusb_ctx == nullptr) {
        if (libusb_init(&g_libusb_ctx) < 0) {
            return false;
        }
    }
    
    if (!pImpl->discoverDevices()) {
        return false;
    }
    
    pImpl->usbThread = std::make_unique<std::thread>(&Impl::usbEventLoop, pImpl.get());
    
    pImpl->initialized = true;
    return true;
}

void LowLatencyDanceGameSDK::shutdown() {
    if (!pImpl->initialized) {
        return;
    }
    
    pImpl->shutdown = true;
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (pImpl->devices[i] && pImpl->devices[i]->transfer) {
            libusb_cancel_transfer(pImpl->devices[i]->transfer);
        }
    }
    
    if (pImpl->usbThread) {
        pImpl->usbThread->join();
        pImpl->usbThread.reset();
    }
    
    pImpl->cleanupDevices();
    pImpl->initialized = false;
}

bool LowLatencyDanceGameSDK::isPlayerConnected(Player player) {
    int idx = static_cast<int>(player);
    return pImpl->devices[idx] && pImpl->devices[idx]->connected;
}

uint16_t LowLatencyDanceGameSDK::getPlayerButtonState(Player player) {
    int idx = static_cast<int>(player);
    if (!pImpl->devices[idx]) {
        return 0;
    }
    return pImpl->devices[idx]->last_button_state;
}

bool LowLatencyDanceGameSDK::isPadCompatible(uint16_t vendor_id, uint16_t product_id) {
    return dance_pad_is_pid_vid_valid_pad(vendor_id, product_id);
}