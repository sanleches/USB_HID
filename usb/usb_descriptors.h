#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

/*
 * =============================================================================
 * USB Descriptors
 * =============================================================================
 * Declarations for USB device, configuration, HID report, and string
 * descriptors. These must be provided to TinyUSB via callbacks.
 *
 * Device identification (VID/PID) is defined in config/hid_config.h.
 *
 * To write a kernel module driver, match on:
 *   - USB_VID (0x1209) and USB_PID (0x0001), and/or
 *   - HID usage page 0xFF00 (vendor-defined)
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * HID report descriptor: vendor-defined page 0xFF00, with:
 *   - Report ID 1 (IN): 32-button bitmask (4 bytes)
 *   - Report ID 2 (OUT): 8-LED bitmask (1 byte)
 */
extern const uint8_t desc_hid_report[];

/*
 * Descriptor accessor callbacks (implemented in usb_descriptors.c).
 * These are invoked by TinyUSB during enumeration.
 */
extern uint8_t const * tud_descriptor_device_cb(void);
extern uint8_t const * tud_descriptor_configuration_cb(uint8_t index);
extern uint8_t const * tud_hid_descriptor_report_cb(uint8_t itf);
extern uint16_t const * tud_descriptor_string_cb(uint8_t index, uint16_t langid);

#ifdef __cplusplus
}
#endif

#endif /* USB_DESCRIPTORS_H */
