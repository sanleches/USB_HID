#include <string.h>
#include "tusb.h"
#include "config/hid_config.h"
#include "hid/hid_handler.h"

//--------------------------------------------------------------------
// Internal State
//--------------------------------------------------------------------

static struct {
    uint8_t button_mask[HID_INPUT_REPORT_SIZE];
    uint8_t led_state;
    bool    dirty;
} _hid;

//--------------------------------------------------------------------
// Public API
//--------------------------------------------------------------------

void hid_handler_init(void) {
    memset(&_hid, 0, sizeof(_hid));
}

void hid_handler_set_button(uint8_t button_idx, bool pressed) {
    if (button_idx >= HID_MAX_BUTTONS) return;

    uint8_t byte_idx = button_idx / 8;
    uint8_t bit_mask = 1u << (button_idx % 8);

    if (pressed) {
        _hid.button_mask[byte_idx] |= bit_mask;
    } else {
        _hid.button_mask[byte_idx] &= ~bit_mask;
    }
    _hid.dirty = true;
}

bool hid_handler_get_button(uint8_t button_idx) {
    if (button_idx >= HID_MAX_BUTTONS) return false;

    uint8_t byte_idx = button_idx / 8;
    uint8_t bit_mask = 1u << (button_idx % 8);

    return (_hid.button_mask[byte_idx] & bit_mask) != 0;
}

bool hid_handler_send_report(void) {
    if (!tud_hid_ready()) return false;

    tud_hid_report(HID_REPORT_ID_INPUT, _hid.button_mask, HID_INPUT_REPORT_SIZE);
    _hid.dirty = false;
    return true;
}

void hid_handler_process_output_report(uint8_t const *data, uint16_t len) {
    if (len > 0) {
        _hid.led_state = data[0];
    }
}

uint8_t hid_handler_get_led_state(void) {
    return _hid.led_state;
}

bool hid_handler_is_ready(void) {
    return tud_ready() && tud_mounted();
}

//--------------------------------------------------------------------
// TinyUSB HID Callbacks
//--------------------------------------------------------------------

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen) {
    (void) itf;
    (void) report_type;

    switch (report_id) {
        case HID_REPORT_ID_INPUT:
            if (reqlen >= HID_INPUT_REPORT_SIZE) {
                memcpy(buffer, _hid.button_mask, HID_INPUT_REPORT_SIZE);
                return HID_INPUT_REPORT_SIZE;
            }
            break;

        case HID_REPORT_ID_OUTPUT:
            if (reqlen >= HID_OUTPUT_REPORT_SIZE) {
                buffer[0] = _hid.led_state;
                return HID_OUTPUT_REPORT_SIZE;
            }
            break;

        default:
            break;
    }
    return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize) {
    (void) itf;
    (void) report_type;

    if (report_id == HID_REPORT_ID_OUTPUT) {
        hid_handler_process_output_report(buffer, bufsize);
    }
}
