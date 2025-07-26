#include "lowlatencydancegamesdk.h"
#include <libusb.h>

struct LowLatencyDanceGameSDK::DanceGameDevice {
    libusb_context *ctx;
    libusb_device_handle *handle;
    uint8_t hid_interface;
    uint8_t interrupt_in_endpoint;
    bool is_initialized;
    uint8_t interrupt_out_endpoint;
};

LowLatencyDanceGameSDK::LowLatencyDanceGameSDK(uint16_t vendor_id, uint16_t product_id) {
    m_isValid = false;
    m_device->is_initialized = false;

    libusb_context *ctx = nullptr;

    if (libusb_init(&ctx) < 0) {
        //LOG->Warn("SMXDirect InputHandler failed to initialize libusb.");
        return;
    }

    // check for smx devices:

    // Open device
    libusb_device_handle *handle = libusb_open_device_with_vid_pid(ctx, vendor_id, product_id);
    if (!handle)
    {
        // LOG->Info("SMXDirect InputHandler: Device not found.");
        libusb_exit(ctx);
        return;
    }

    // Find the correct HID interface
    struct libusb_config_descriptor *config;
    libusb_get_active_config_descriptor(libusb_get_device(handle), &config);

    int hid_interface = -1;
    int hid_interface_index = -1;
    for (int i = 0; i < config->bNumInterfaces; i++) {
        const struct libusb_interface_descriptor *intf = &config->interface[i].altsetting[0];
        if (intf->bInterfaceClass == 3) { // HID class = 3
            hid_interface = intf->bInterfaceNumber;  // Use the actual interface number
            hid_interface_index = i;  // Keep track of the array index for endpoint discovery
            /* LOG->Info("SMXDirect InputHandler: Found HID interface: %d (index=%d, class=%d, subclass=%d, protocol=%d)", 
                hid_interface, 
                i, 
                (int)intf->bInterfaceClass, 
                (int)intf->bInterfaceSubClass, 
                (int)intf->bInterfaceProtocol); */
            break;
        }
    }

    if (hid_interface == -1) {
        // LOG->Warn("SMXDirect InputHandler: No HID interface found!");
        libusb_free_config_descriptor(config);
        libusb_close(handle);
        libusb_exit(ctx);
        return;
    }

    // Detach kernel driver if active
    if (libusb_kernel_driver_active(handle, hid_interface) == 1)
    {
        // LOG->Info("SMXDirect InputHandler: Kernel driver is active on interface %d, detaching...", hid_interface);
        int ret = libusb_detach_kernel_driver(handle, hid_interface);
        if (ret != 0)
        {
            // LOG->Warn("SMXDirect InputHandler: Failed to detach kernel driver: %s", libusb_error_name(ret));
            return;
        }
    }

    // Claim the HID interface
    if (libusb_claim_interface(handle, hid_interface) < 0)
    {
        // LOG->Warn("SMXDirect InputHandler: Failed to claim HID interface %d", hid_interface);
        libusb_free_config_descriptor(config);
        libusb_close(handle);
        libusb_exit(ctx);
        return;
    }

    // LOG->Info("SMXDirect InputHandler: Connected! Discovering endpoints...");

    // Discover endpoints on the HID interface
    uint8_t interrupt_in_endpoint = 0;
    uint8_t interrupt_out_endpoint = 0;

    for (int i = 0; i < config->interface[hid_interface_index].altsetting[0].bNumEndpoints; i++)
    {
        const struct libusb_endpoint_descriptor *ep = &config->interface[hid_interface_index].altsetting[0].endpoint[i];
        
        bool is_interrupt = (ep->bmAttributes & 0x03) == 0x03;
        bool is_input = (ep->bEndpointAddress & 0x80) != 0;
        bool is_output = (ep->bEndpointAddress & 0x80) == 0;
        
        /* LOG->Info("SMXDirect InputHandler: Endpoint 0x%02x: %s, %s, max packet size: %d",
               ep->bEndpointAddress,
               is_input ? "IN" : "OUT",
               is_interrupt ? "Interrupt" : "Other",
               ep->wMaxPacketSize); */
        
        if (is_interrupt && is_input && interrupt_in_endpoint == 0) {
            interrupt_in_endpoint = ep->bEndpointAddress;
            // LOG->Info("Using IN endpoint: 0x%02x", (int)interrupt_in_endpoint);
        }

        if (is_interrupt && is_output && interrupt_out_endpoint == 0) {
            interrupt_out_endpoint = ep->bEndpointAddress;
            // LOG->Info("Found OUT endpoint: 0x%02x", (int)interrupt_out_endpoint);
        }
    }

    libusb_free_config_descriptor(config);

    if (interrupt_in_endpoint == 0) {
        // LOG->Warn("SMXDirect InputHandler: No interrupt IN endpoint found!");
        libusb_release_interface(handle, hid_interface);
        libusb_close(handle);
        libusb_exit(ctx);
        return;
    }

    m_isValid = true;

    m_device->ctx = ctx;
    m_device->handle = handle;
    m_device->hid_interface = hid_interface;
    m_device->interrupt_in_endpoint = interrupt_in_endpoint;
    m_device->interrupt_out_endpoint = interrupt_out_endpoint;
    m_device->is_initialized = true;
}

LowLatencyDanceGameSDK::~LowLatencyDanceGameSDK() {
    if (m_device->is_initialized) {
        libusb_release_interface(m_device->handle, m_device->hid_interface);
        libusb_close(m_device->handle);
        libusb_exit(m_device->ctx);
    }
}

bool LowLatencyDanceGameSDK::is_valid() const {
    return m_isValid;
}

int LowLatencyDanceGameSDK::read_data(uint8_t* buffer, int expectedCount) {
    int bytes_read = 0;
    int result = libusb_interrupt_transfer(
        m_device->handle,
        m_device->interrupt_in_endpoint,
        buffer,
        expectedCount,
        &bytes_read,
        0); // never time out

    if (result < 0) {
        return result;
    }
    
    return bytes_read;
}