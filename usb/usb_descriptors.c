#include "tusb.h"
#include "bsp/board_api.h"
#include "config/hid_config.h"
#include "usb/usb_descriptors.h"

//--------------------------------------------------------------------
// HID Report Descriptor
//--------------------------------------------------------------------
// Vendor-defined (Usage Page 0xFF00) with two report IDs:
//   ID 1 (IN): 32 buttons as bitmask
//   ID 2 (OUT): 8 LEDs as bitmask

const uint8_t desc_hid_report[] = {
    0x06, 0x00, 0xFF,          // Usage Page (Vendor Page 0xFF00)
    0x09, 0x01,                // Usage (Vendor Usage 0x01 - Stream Deck)
    0xA1, 0x01,                // Collection (Application)

    // --- Input: Button State Bitmask (Report ID 1) ---
    0x85, HID_REPORT_ID_INPUT, //   Report ID (1)
    0x15, 0x00,                //   Logical Minimum (0)
    0x25, 0x01,                //   Logical Maximum (1)
    0x75, 0x01,                //   Report Size (1 bit per button)
    0x95, HID_INPUT_BUTTON_COUNT, // Report Count (32 buttons)
    0x09, 0x01,                //   Usage (Button Array)
    0x81, 0x02,                //   Input (Data,Var,Abs)

    // --- Output: LED State Bitmask (Report ID 2) ---
    0x85, HID_REPORT_ID_OUTPUT,//   Report ID (2)
    0x15, 0x00,                //   Logical Minimum (0)
    0x25, 0x01,                //   Logical Maximum (1)
    0x75, 0x01,                //   Report Size (1 bit per LED)
    0x95, HID_OUTPUT_LED_COUNT,//   Report Count (8 LEDs)
    0x09, 0x02,                //   Usage (LED Array)
    0x91, 0x02,                //   Output (Data,Var,Abs)

    0xC0                       // End Collection
};

//--------------------------------------------------------------------
// Device Descriptor
//--------------------------------------------------------------------

tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = USB_BCD_DEVICE,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------
// HID Report Descriptor Callback
//--------------------------------------------------------------------

uint8_t const * tud_hid_descriptor_report_cb(uint8_t itf) {
    (void) itf;
    return desc_hid_report;
}

//--------------------------------------------------------------------
// Configuration Descriptor
//--------------------------------------------------------------------

enum {
    ITF_NUM_HID,
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

#define EPNUM_HID         0x01

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE,
                             sizeof(desc_hid_report),
                             EPNUM_HID, 0x80 | EPNUM_HID,
                             CFG_TUD_HID_EP_BUFSIZE, 10)
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return desc_configuration;
}

//--------------------------------------------------------------------
// String Descriptors
//--------------------------------------------------------------------

enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
};

char const *string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },  // LANGID: English (0x0409)
    USB_MANUFACTURER_STR,
    USB_PRODUCT_STR,
    USB_SERIAL_STR,
};

static uint16_t _desc_str[32 + 1];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    size_t chr_count;

    switch (index) {
        case STRID_LANGID:
            memcpy(&_desc_str[1], string_desc_arr[0], 2);
            chr_count = 1;
            break;

        case STRID_SERIAL:
            chr_count = board_get_unique_id((uint8_t *)(_desc_str + 1), 32);
            break;

        default:
            if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
                return NULL;

            const char *str = string_desc_arr[index];
            chr_count = strlen(str);
            size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1;
            if (chr_count > max_count) chr_count = max_count;

            for (size_t i = 0; i < chr_count; i++) {
                _desc_str[1 + i] = str[i];
            }
            break;
    }

    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}
