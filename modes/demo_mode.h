#ifndef DEMO_MODE_H
#define DEMO_MODE_H

/*
 * =============================================================================
 * Demo Mode
 * =============================================================================
 * Automatically cycles through a set of button indices, pressing and
 * releasing them in sequence. Useful for testing the HID path without
 * physical buttons connected.
 *
 * Behavior:
 *   - Selects one button at a time from DEMO_BUTTON_INDICES
 *   - Holds it pressed for DEMO_PRESS_INTERVAL_MS
 *   - Releases it and moves to the next
 *   - Loops back to the start after the last button
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize demo mode state.
 */
void demo_mode_init(void);

/*
 * Called periodically from the main loop.
 * Updates the auto-cycle state machine and sets button states
 * via hid_handler_set_button().
 */
void demo_mode_update(void);

/*
 * Reset demo mode (release all buttons, restart cycle).
 */
void demo_mode_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* DEMO_MODE_H */
