#ifndef BUTTON_H
#define BUTTON_H

/*
 * =============================================================================
 * Button GPIO Abstraction
 * =============================================================================
 * Provides debounced reading of physical buttons connected to GPIO pins.
 * Uses the Pico's internal pull-up resistors so buttons connect the pin
 * to GND when pressed (active-low).
 *
 * Debounce logic: a button state is accepted only after it remains stable
 * for DEBOUNCE_MS consecutive milliseconds.
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Configuration struct for a bank of buttons.
 * The pin array maps directly to HID button indices:
 *   button 0 -> pins[0], button 1 -> pins[1], etc.
 */
typedef struct {
    const uint8_t *gpio_pins;   /* Array of GPIO pin numbers */
    uint8_t        count;       /* Number of buttons */
} button_config_t;

/*
 * Opaque button state handle.
 */
typedef struct button_state button_state_t;

/*
 * Initialize a button bank.
 * Configures all specified GPIO pins as inputs with pull-up.
 * @param config  Button configuration (pins and count). Must remain valid.
 * @return Handle for subsequent calls, or NULL on failure.
 */
button_state_t *button_init(const button_config_t *config);

/*
 * Read and debounce all buttons in the bank.
 * Must be called periodically (e.g., every USB_POLL_INTERVAL_MS).
 * Updates internal debounce state.
 */
void button_update(button_state_t *state);

/*
 * Get the debounced state of a specific button.
 * @param state  Button bank handle
 * @param idx    Button index (0..config.count-1)
 * @return true if the button is currently pressed (debounced)
 */
bool button_is_pressed(const button_state_t *state, uint8_t idx);

/*
 * Get the debounced state bitmask for all buttons.
 * @param state  Button bank handle
 * @return Bitmask: bit N = 1 if button N pressed
 */
uint32_t button_get_mask(const button_state_t *state);

/*
 * Clean up button state (free resources).
 * @param state  Button bank handle
 */
void button_deinit(button_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H */
