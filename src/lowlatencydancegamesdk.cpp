#include "lowlatencydancegamesdk.h"
#include <libusb.h>

// SMX vendor/product IDs
static constexpr uint16_t SMX_VENDOR_ID = 0x2341;
static constexpr uint16_t SMX_PRODUCT_ID = 0x8037;

// Global libusb context for device management
static libusb_context* g_libusb_ctx = nullptr;

struct LowLatencyDanceGameSDK::DanceGameDevice {
    libusb_device_handle *handle;
    uint8_t hid_interface;
    uint8_t interrupt_in_endpoint;
    uint8_t interrupt_out_endpoint;
    bool is_initialized;
};

// Static methods implementation
bool LowLatencyDanceGameSDK::initialize() {
    if (g_libusb_ctx != nullptr) {
        return true; // Already initialized
    }
    
    if (libusb_init(&g_libusb_ctx) < 0) {
        return false;
    }
    
    return true;
}

void LowLatencyDanceGameSDK::shutdown() {
    if (g_libusb_ctx != nullptr) {
        libusb_exit(g_libusb_ctx);
        g_libusb_ctx = nullptr;
    }
}

// Helper method to setup device from libusb handle
bool LowLatencyDanceGameSDK::setup_device_from_handle(void* handle_ptr, DanceGameDevice* device) {
    libusb_device_handle* handle = static_cast<libusb_device_handle*>(handle_ptr);
    // Find the correct HID interface
    struct libusb_config_descriptor *config;
    libusb_get_active_config_descriptor(libusb_get_device(handle), &config);

    int hid_interface = -1;
    int hid_interface_index = -1;
    for (int i = 0; i < config->bNumInterfaces; i++) {
        const struct libusb_interface_descriptor *intf = &config->interface[i].altsetting[0];
        if (intf->bInterfaceClass == 3) { // HID class = 3
            hid_interface = intf->bInterfaceNumber;
            hid_interface_index = i;
            break;
        }
    }

    if (hid_interface == -1) {
        libusb_free_config_descriptor(config);
        return false;
    }

    // Detach kernel driver if active
    if (libusb_kernel_driver_active(handle, hid_interface) == 1) {
        int ret = libusb_detach_kernel_driver(handle, hid_interface);
        if (ret != 0) {
            libusb_free_config_descriptor(config);
            return false;
        }
    }

    // Claim the HID interface
    if (libusb_claim_interface(handle, hid_interface) < 0) {
        libusb_free_config_descriptor(config);
        return false;
    }

    // Discover endpoints on the HID interface
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
    device->hid_interface = hid_interface;
    device->interrupt_in_endpoint = interrupt_in_endpoint;
    device->interrupt_out_endpoint = interrupt_out_endpoint;
    device->is_initialized = true;

    return true;
}

// Helper method to determine player number via vendor protocol
int LowLatencyDanceGameSDK::get_player_number_from_device(void* handle_ptr, uint8_t interrupt_in_endpoint, uint8_t interrupt_out_endpoint) {
    libusb_device_handle* handle = static_cast<libusb_device_handle*>(handle_ptr);
    // Request device info: { 5, 0x80, 0 }
    const unsigned char data[] = { 5, 0x80, 0 };
    int bytes_sent = 0;
    
    if (interrupt_out_endpoint != 0) {
        int result = libusb_interrupt_transfer(handle, interrupt_out_endpoint, 
                                               (unsigned char*)data, sizeof(data), 
                                               &bytes_sent, 1000);
        if (result < 0) {
            return 0; // Default to P1 on error
        }
    }
    
    // Read response using the correct interrupt IN endpoint
    unsigned char buf[65];
    int bytes_read = 0;
    int result = libusb_interrupt_transfer(handle, interrupt_in_endpoint,
                                           buf, sizeof(buf), 
                                           &bytes_read, 1000);
    
    if (result < 0 || bytes_read < 4) {
        return 0; // Default to P1 on error
    }
    
    // Pad is char '1' if P2, or char '0' if P1 (byte index 3)
    return (buf[3] == '1') ? 1 : 0;
}

LowLatencyDanceGameSDK** LowLatencyDanceGameSDK::discover_devices() {
    if (g_libusb_ctx == nullptr) {
        return nullptr;
    }
    
    // Allocate result array
    LowLatencyDanceGameSDK** result = new LowLatencyDanceGameSDK*[MAX_PADS];
    result[0] = nullptr;
    result[1] = nullptr;
    
    // Get device list
    libusb_device **device_list;
    ssize_t device_count = libusb_get_device_list(g_libusb_ctx, &device_list);
    
    if (device_count < 0) {
        delete[] result;
        return nullptr;
    }
    
    int found_devices = 0;
    
    // Find all SMX devices
    for (ssize_t i = 0; i < device_count && found_devices < MAX_PADS; i++) {
        struct libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(device_list[i], &desc) < 0) {
            continue;
        }
        
        if (desc.idVendor == SMX_VENDOR_ID && desc.idProduct == SMX_PRODUCT_ID) {
            libusb_device_handle *handle;
            if (libusb_open(device_list[i], &handle) < 0) {
                continue;
            }
            
            // Create new instance
            LowLatencyDanceGameSDK* instance = new LowLatencyDanceGameSDK();
            instance->m_device = new DanceGameDevice();
            instance->m_isValid = false;
            instance->m_playerNumber = 0;
            
            if (setup_device_from_handle(handle, instance->m_device)) {
                // Determine player number via vendor protocol
                int player = get_player_number_from_device(handle, 
                                                         instance->m_device->interrupt_in_endpoint,
                                                         instance->m_device->interrupt_out_endpoint);
                
                instance->m_playerNumber = player;
                instance->m_isValid = true;
                
                // Place in correct array slot
                if (player >= 0 && player < MAX_PADS && result[player] == nullptr) {
                    result[player] = instance;
                    found_devices++;
                } else {
                    // Duplicate or invalid player number, clean up
                    delete instance;
                    libusb_close(handle);
                }
            } else {
                delete instance;
                libusb_close(handle);
            }
        }
    }
    
    libusb_free_device_list(device_list, 1);
    return result;
}

// Private constructor
LowLatencyDanceGameSDK::LowLatencyDanceGameSDK() : m_device(nullptr), m_isValid(false), m_playerNumber(0) {
}

LowLatencyDanceGameSDK::~LowLatencyDanceGameSDK() {
    if (m_device && m_device->is_initialized) {
        libusb_release_interface(m_device->handle, m_device->hid_interface);
        libusb_close(m_device->handle);
    }

    delete m_device;
}

bool LowLatencyDanceGameSDK::is_valid() const {
    return m_isValid;
}

int LowLatencyDanceGameSDK::get_player_number() const {
    return m_playerNumber;
}

int LowLatencyDanceGameSDK::read_data(uint8_t* buffer, int expectedCount) {
    int bytes_read = 0;
    int result = libusb_interrupt_transfer(
        m_device->handle,
        m_device->interrupt_in_endpoint,
        buffer,
        expectedCount,
        &bytes_read,
        1000); // milliseconds: Timeout once per second to allow for run loop to interate if the device doesn't initialize correctly

    if (result < 0) {
        return result;
    }
    
    return bytes_read;
}