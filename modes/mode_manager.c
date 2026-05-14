#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "config/hid_config.h"
#include "modes/mode_manager.h"
#include "modes/demo_mode.h"
#include "modes/normal_mode.h"

//--------------------------------------------------------------------
// Internal State
//--------------------------------------------------------------------

static mode_t _current_mode = MODE_DEMO;

//--------------------------------------------------------------------
// Mode Detection
//--------------------------------------------------------------------

static mode_t read_mode_select(void) {
    gpio_init(MODE_SELECT_GPIO);
    gpio_set_dir(MODE_SELECT_GPIO, GPIO_IN);
    gpio_pull_down(MODE_SELECT_GPIO);

    if (gpio_get(MODE_SELECT_GPIO)) {
        return MODE_NORMAL;
    }
    return MODE_DEMO;
}

//--------------------------------------------------------------------
// Public API
//--------------------------------------------------------------------

mode_t mode_manager_init(void) {
    _current_mode = read_mode_select();

    switch (_current_mode) {
        case MODE_NORMAL:
            normal_mode_init();
            break;

        case MODE_DEMO:
        default:
            demo_mode_init();
            break;
    }

    return _current_mode;
}

void mode_manager_update(void) {
    switch (_current_mode) {
        case MODE_NORMAL:
            normal_mode_update();
            break;

        case MODE_DEMO:
        default:
            demo_mode_update();
            break;
    }
}

mode_t mode_manager_get_current_mode(void) {
    return _current_mode;
}

const char *mode_manager_get_mode_name(void) {
    switch (_current_mode) {
        case MODE_NORMAL: return "NORMAL";
        case MODE_DEMO:   return "DEMO";
        default:          return "UNKNOWN";
    }
}
