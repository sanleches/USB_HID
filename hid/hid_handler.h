#ifndef HID_HANDLER_H
#define HID_HANDLER_H

/*
 * =============================================================================
 * HID Report Handler
 * =============================================================================
 * Builds and sends HID input reports containing the button state bitmask.
 * Processes received HID output reports (LED state).
 *
 * The input report format (Report ID 1):
 *   [4 bytes] - 32-bit button state bitmask
 *     Byte 0: buttons 0-7 (bit 0 = button 0, bit 7 = button 7)
 *     Byte 1: buttons 8-15
 *     Byte 2: buttons 16-23
 *     Byte 3: buttons 24-31
 *
 * The output report format (Report ID 2):
 *   [1 byte] - 8-bit LED state bitmask
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Maximum number of buttons in the HID report.
 * Defined by the HID report descriptor.
 */
#define HID_MAX_BUTTONS  32

/*
 * Initialize the HID handler state (all buttons released).
 */
void hid_handler_init(void);

/*
 * Set the state of a single button.
 * @param button_idx  Button index (0..HID_MAX_BUTTONS-1)
 * @param pressed     true = pressed, false = released
 */
void hid_handler_set_button(uint8_t button_idx, bool pressed);

/*
 * Get the current state of a single button.
 * @param button_idx  Button index (0..HID_MAX_BUTTONS-1)
 * @return true if pressed
 */
bool hid_handler_get_button(uint8_t button_idx);

/*
 * Send the current button state as a HID input report.
 * Called periodically from the main loop.
 * @return true if the report was sent, false if USB is not ready
 */
bool hid_handler_send_report(void);

/*
 * Process a received HID output report (LED state from host).
 * @param data   Pointer to received data (without report ID)
 * @param len    Length of received data in bytes
 */
void hid_handler_process_output_report(uint8_t const *data, uint16_t len);

/*
 * Get the current LED state bitmask received from the host.
 * @return 8-bit LED state bitmask
 */
uint8_t hid_handler_get_led_state(void);

/*
 * Check if USB is ready and mounted.
 */
bool hid_handler_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* HID_HANDLER_H */
