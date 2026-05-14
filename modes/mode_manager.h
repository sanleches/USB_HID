#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

/*
 * =============================================================================
 * Mode Manager
 * =============================================================================
 * Selects and manages the two operational modes:
 *
 *   DEMO mode   (MODE_SELECT_GPIO = LOW):
 *     Auto-cycles through 4 virtual buttons for testing without hardware.
 *     Blinks LED slowly (1000ms) to indicate demo mode.
 *
 *   NORMAL mode (MODE_SELECT_GPIO = HIGH):
 *     Reads physical buttons connected to GPIO pins with pull-down resistors.
 *     Blinks LED quickly (100ms) to indicate normal mode.
 *
 * Mode is read at startup from the MODE_SELECT_GPIO pin, which has a
 * pull-down resistor. If nothing is connected, default is DEMO mode.
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Operational modes.
 */
typedef enum {
    MODE_DEMO   = 0,
    MODE_NORMAL = 1,
} mode_t;

/*
 * Read the mode select GPIO and initialize the corresponding mode.
 * Called once at startup after USB and HID are initialized.
 * @return The selected mode.
 */
mode_t mode_manager_init(void);

/*
 * Called periodically from the main loop.
 * Delegates to the currently active mode's update function.
 */
void mode_manager_update(void);

/*
 * Get the currently active mode.
 */
mode_t mode_manager_get_current_mode(void);

/*
 * Get a human-readable name for the current mode.
 */
const char *mode_manager_get_mode_name(void);

#ifdef __cplusplus
}
#endif

#endif /* MODE_MANAGER_H */
