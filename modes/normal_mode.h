#ifndef NORMAL_MODE_H
#define NORMAL_MODE_H

/*
 * =============================================================================
 * Normal Mode
 * =============================================================================
 * Reads physical buttons using the button abstraction layer and forwards
 * their states to the HID handler. Button GPIO pins use internal pull-down
 * resistors (button connects pin to 3.3V when pressed).
 *
 * GPIO-to-HID mapping (from config/hid_config.h):
 *   NORMAL_BUTTON_GPIO_PINS[N] -> HID button index N
 */

#include <stdint.h>
#include <stdbool.h>
#include "board/button.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize normal mode: configures button GPIO pins with pull-down.
 * Must be called before normal_mode_update().
 */
void normal_mode_init(void);

/*
 * Called periodically from the main loop.
 * Reads and debounces all physical buttons, forwarding state changes
 * to hid_handler_set_button().
 */
void normal_mode_update(void);

/*
 * Clean up normal mode resources.
 */
void normal_mode_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* NORMAL_MODE_H */
