#include <stdlib.h>
#include "config/hid_config.h"
#include "hid/hid_handler.h"
#include "board/button.h"
#include "modes/normal_mode.h"

//--------------------------------------------------------------------
// Internal State
//--------------------------------------------------------------------

static const uint8_t normal_gpio_pins[NORMAL_BUTTON_COUNT] = NORMAL_BUTTON_GPIO_PINS;

static const button_config_t normal_button_config = {
    .gpio_pins = normal_gpio_pins,
    .count     = NORMAL_BUTTON_COUNT
};

static button_state_t *_button_state = NULL;
static uint32_t        _prev_mask = 0;

//--------------------------------------------------------------------
// Public API
//--------------------------------------------------------------------

void normal_mode_init(void) {
    _button_state = button_init(&normal_button_config);
    _prev_mask = 0;
}

void normal_mode_update(void) {
    if (!_button_state) return;

    button_update(_button_state);
    uint32_t mask = button_get_mask(_button_state);

    /* Only report changes to minimize USB traffic */
    uint32_t changed = mask ^ _prev_mask;

    if (changed == 0) return;

    for (uint8_t i = 0; i < NORMAL_BUTTON_COUNT; i++) {
        uint32_t bit = (1u << i);
        if (changed & bit) {
            hid_handler_set_button(i, (mask & bit) != 0);
        }
    }

    _prev_mask = mask;
}

void normal_mode_deinit(void) {
    button_deinit(_button_state);
    _button_state = NULL;
    _prev_mask = 0;
}
