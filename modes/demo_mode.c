#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "config/hid_config.h"
#include "hid/hid_handler.h"
#include "modes/demo_mode.h"

//--------------------------------------------------------------------
// Internal State
//--------------------------------------------------------------------

static const uint8_t demo_buttons[DEMO_BUTTON_COUNT] = DEMO_BUTTON_INDICES;

static struct {
    uint8_t current_index;    /* Index into demo_buttons array */
    bool    is_pressed;
    absolute_time_t next_toggle_time;
} _demo;

//--------------------------------------------------------------------
// Public API
//--------------------------------------------------------------------

void demo_mode_init(void) {
    _demo.current_index = 0;
    _demo.is_pressed = false;
    _demo.next_toggle_time = make_timeout_time_ms(DEMO_PRESS_INTERVAL_MS);
}

void demo_mode_update(void) {
    if (absolute_time_diff_us(get_absolute_time(), _demo.next_toggle_time) > 0) {
        return;
    }

    _demo.next_toggle_time = make_timeout_time_ms(DEMO_PRESS_INTERVAL_MS);

    if (_demo.is_pressed) {
        /* Release current button */
        hid_handler_set_button(demo_buttons[_demo.current_index], false);

        /* Move to next button */
        _demo.current_index = (_demo.current_index + 1) % DEMO_BUTTON_COUNT;
        _demo.is_pressed = false;
    } else {
        /* Press current button */
        hid_handler_set_button(demo_buttons[_demo.current_index], true);
        _demo.is_pressed = true;
    }
}

void demo_mode_reset(void) {
    /* Release all demo buttons */
    for (uint8_t i = 0; i < DEMO_BUTTON_COUNT; i++) {
        hid_handler_set_button(demo_buttons[i], false);
    }
    _demo.current_index = 0;
    _demo.is_pressed = false;
    _demo.next_toggle_time = make_timeout_time_ms(DEMO_PRESS_INTERVAL_MS);
}
