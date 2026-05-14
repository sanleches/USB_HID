#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "config/hid_config.h"
#include "board/button.h"

//--------------------------------------------------------------------
// Internal Types
//--------------------------------------------------------------------

struct button_state {
    const button_config_t *config;

    uint32_t *debounced;         /* Stable (debounced) state */
    uint32_t *raw_history;       /* Previous raw read for edge detection */
    uint32_t *debounce_counters; /* Millisecond counters for debounce */
};

//--------------------------------------------------------------------
// Implementation
//--------------------------------------------------------------------

button_state_t *button_init(const button_config_t *config) {
    if (!config || !config->gpio_pins || config->count == 0) return NULL;

    button_state_t *state = calloc(1, sizeof(button_state_t));
    if (!state) return NULL;

    state->config = config;

    state->debounced = calloc(1, sizeof(uint32_t));
    state->raw_history = calloc(1, sizeof(uint32_t));
    state->debounce_counters = calloc(config->count, sizeof(uint32_t));

    if (!state->debounced || !state->raw_history || !state->debounce_counters) {
        button_deinit(state);
        return NULL;
    }

    for (uint8_t i = 0; i < config->count; i++) {
        uint8_t pin = config->gpio_pins[i];
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_down(pin);
    }

    return state;
}

void button_update(button_state_t *state) {
    if (!state) return;

    const button_config_t *cfg = state->config;
    uint32_t raw = 0;

    for (uint8_t i = 0; i < cfg->count; i++) {
        if (gpio_get(cfg->gpio_pins[i])) {
            raw |= (1u << i);
        }
    }

    uint32_t changed = raw ^ (*state->raw_history);
    *state->raw_history = raw;

    for (uint8_t i = 0; i < cfg->count; i++) {
        uint32_t bit = (1u << i);

        if (!(changed & bit)) {
            /* No change: reset debounce counter */
            state->debounce_counters[i] = 0;
            continue;
        }

        bool current_raw = (raw & bit) != 0;
        bool current_deb = (*state->debounced & bit) != 0;

        if (current_raw != current_deb) {
            state->debounce_counters[i]++;
            if (state->debounce_counters[i] >= DEBOUNCE_MS) {
                if (current_raw) {
                    *state->debounced |= bit;
                } else {
                    *state->debounced &= ~bit;
                }
                state->debounce_counters[i] = 0;
            }
        } else {
            state->debounce_counters[i] = 0;
        }
    }
}

bool button_is_pressed(const button_state_t *state, uint8_t idx) {
    if (!state || idx >= state->config->count) return false;
    return (*state->debounced & (1u << idx)) != 0;
}

uint32_t button_get_mask(const button_state_t *state) {
    if (!state) return 0;
    return *state->debounced;
}

void button_deinit(button_state_t *state) {
    if (!state) return;
    free(state->debounced);
    free(state->raw_history);
    free(state->debounce_counters);
    free(state);
}

//--------------------------------------------------------------------
// Convenience: direct GPIO read (no debounce) for mode select pin
//--------------------------------------------------------------------

bool gpio_read_with_pulldown(uint8_t pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_down(pin);
    return gpio_get(pin);
}
