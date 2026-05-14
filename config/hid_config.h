#ifndef HID_CONFIG_H
#define HID_CONFIG_H

/*
 * =============================================================================
 * USB Device Identification
 * =============================================================================
 * These values are used by the kernel module to identify this device.
 * VID 0x1209 is in the "Generic USB PID" range (prototype/development use).
 * Change these to match your kernel module's expected values.
 */
#define USB_VID                  0x6666
#define USB_PID                  0x0001
#define USB_BCD_DEVICE           0x0100

#define USB_MANUFACTURER_STR     "USB HID Project"
#define USB_PRODUCT_STR          "Custom Button emulation v1"
#define USB_SERIAL_STR           "06071998"

/*
 * =============================================================================
 * HID Report Layout
 * =============================================================================
 * The device presents as a vendor-defined HID (Usage Page 0xFF00).
 * 
 * Report ID 1 (IN - device to host): Button state bitmask
 *   - 32 buttons, each as a single bit (1 = pressed, 0 = released)
 *   - Report length: 4 bytes
 *   - Byte 0: buttons 0-7, Byte 1: buttons 8-15, etc.
 * 
 * Report ID 2 (OUT - host to device): LED state bitmask (future use)
 *   - 8 LEDs, each as a single bit (1 = on, 0 = off)
 *   - Report length: 1 byte
 */
#define HID_INPUT_BUTTON_COUNT   32
#define HID_INPUT_REPORT_SIZE    4

#define HID_OUTPUT_LED_COUNT     8
#define HID_OUTPUT_REPORT_SIZE   1

#define HID_REPORT_ID_INPUT      1
#define HID_REPORT_ID_OUTPUT     2

/*
 * =============================================================================
 * Mode Selection
 * =============================================================================
 * GPIO pin used to select operational mode:
 *   LOW  (pulled down)  -> DEMO mode (auto-cycles button presses)
 *   HIGH (3.3V)         -> NORMAL mode (reads physical buttons)
 */
#define MODE_SELECT_GPIO         14

/*
 * =============================================================================
 * Demo Mode Configuration
 * =============================================================================
 * In demo mode, buttons are pressed/released in sequence automatically.
 * DEMO_BUTTON_INDICES: which HID button indices to cycle through.
 * DEMO_PRESS_INTERVAL_MS: time between button state changes.
 */
#define DEMO_BUTTON_COUNT        4
#define DEMO_BUTTON_INDICES      {0, 1, 2, 3}
#define DEMO_PRESS_INTERVAL_MS   1000

/*
 * =============================================================================
 * Normal Mode Button Configuration
 * =============================================================================
 * Maps physical GPIO pins to HID button indices.
 * Uses internal pull-down resistors (button connects pin to 3.3V when pressed).
 * NORMAL_BUTTON_COUNT: number of physical buttons.
 * NORMAL_BUTTON_GPIO_PINS: GPIO pins (order maps to HID indices 0..N-1).
 */
#define NORMAL_BUTTON_COUNT       8
#define NORMAL_BUTTON_GPIO_PINS  {2, 3, 4, 5, 6, 7, 8, 9}

/*
 * =============================================================================
 * Debounce Configuration
 * =============================================================================
 * Debounce window in milliseconds. A button state must be stable for this
 * duration before being accepted.
 */
#define DEBOUNCE_MS              25

/*
 * =============================================================================
 * Status LED (Pico W on-board LED)
 * =============================================================================
 * Blink patterns:
 *   Fast (100ms)  : USB not mounted
 *   Slow (1000ms) : USB mounted, running
 */
#define LED_BLINK_NOT_MOUNTED_MS  100
#define LED_BLINK_MOUNTED_MS      1000
#define LED_BLINK_SUSPENDED_MS    2500

/*
 * =============================================================================
 * USB Polling Interval
 * =============================================================================
 * How often (in ms) the main loop scans buttons and sends HID reports.
 */
#define USB_POLL_INTERVAL_MS     10

#endif /* HID_CONFIG_H */
