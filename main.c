/*
 * =============================================================================
 * Custom Stream Deck - USB HID Button Controller
 * =============================================================================
 * 
 * Board:  Raspberry Pi Pico 2 W (RP2350)
 * Stack:  Pico SDK 2.2.0 + TinyUSB 0.18.0
 * 
 * This device emulates a Stream Deck-like USB HID controller with 32 virtual
 * buttons. It presents as a vendor-defined HID device (Usage Page 0xFF00).
 * 
 * Modes:
 *   DEMO    (GPIO 14 = LOW)  - Auto-cycles button presses for testing
 *   NORMAL  (GPIO 14 = HIGH) - Reads physical buttons via GPIO
 * 
 * Device identification for kernel module:
 *   VID: 0x1209  PID: 0x0001
 *   HID Usage Page: 0xFF00 (Vendor-defined)
 * 
 * Protocol:
 *   Report ID 1 (IN): 32-bit button state bitmask (4 bytes)
 *   Report ID 2 (OUT): 8-bit LED state bitmask (1 byte)
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "tusb.h"
#include "bsp/board_api.h"

#include "config/hid_config.h"
#include "hid/hid_handler.h"
#include "modes/mode_manager.h"

//--------------------------------------------------------------------
// Status LED Task
//--------------------------------------------------------------------

static uint32_t _led_blink_interval_ms = LED_BLINK_NOT_MOUNTED_MS;
static uint32_t _led_last_toggle_ms = 0;
static bool     _led_state = false;

static void led_blinking_task(void) {
    uint32_t now = board_millis();
    if (now - _led_last_toggle_ms < _led_blink_interval_ms) return;
    _led_last_toggle_ms = now;

    _led_state = !_led_state;

#ifdef CYW43_WL_GPIO_LED_PIN
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, _led_state);
#else
    (void)_led_state;
#endif
}

//--------------------------------------------------------------------
// USB Device Callbacks
//--------------------------------------------------------------------

void tud_mount_cb(void) {// Called when the USB host successfully mounts the device
    _led_blink_interval_ms = LED_BLINK_MOUNTED_MS;// Set LED to solid on when mounted
}

void tud_umount_cb(void) {  // Called when the USB host unmounts the device
    _led_blink_interval_ms = LED_BLINK_NOT_MOUNTED_MS;
}

void tud_suspend_cb(bool remote_wakeup_en) {// Called when the USB host suspends the device (e.g. PC goes to sleep)
    (void) remote_wakeup_en;
    _led_blink_interval_ms = LED_BLINK_SUSPENDED_MS;
}

void tud_resume_cb(void) {
    _led_blink_interval_ms = tud_mounted() ? LED_BLINK_MOUNTED_MS
                                           : LED_BLINK_NOT_MOUNTED_MS;
}

//--------------------------------------------------------------------
// Main
//--------------------------------------------------------------------

int main(void) {
    /*
     * Initialize system clock, stdio (UART), etc.
     */
    board_init();

    /*
     * Initialize the CYW43 WiFi chip (needed for the Pico W on-board LED).
     * This is safe even on non-W variants with pico_cyw43_arch_none.
     */
    if (cyw43_arch_init()) {
        printf("CYW43 init failed\n");
    }

    /*
     * Initialize HID state and TinyUSB device stack.
     */
    hid_handler_init();

    tusb_rhport_init_t dev_init = {
        .role  = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    tusb_init(0, &dev_init);

    if (board_init_after_tusb) {
        board_init_after_tusb();
    }

    /*
     * Read mode select GPIO and initialize the appropriate mode.
     */
    mode_t mode = mode_manager_init();

    printf("Custom Stream Deck v1\n");
    printf("Mode: %s\n", mode_manager_get_mode_name());
    printf("VID: 0x%04X  PID: 0x%04X\n", USB_VID, USB_PID);

    /*
     * Main loop
     */
    uint32_t last_poll_ms = 0;

    while (true) {
        /*
         * Handle TinyUSB device tasks (must be called frequently).
         */
        tud_task();

        /*
         * Blink status LED based on USB mount state.
         */
        led_blinking_task();

        /*
         * Poll buttons and send HID report at the configured interval.
         */
        uint32_t now_ms = board_millis();
        if (now_ms - last_poll_ms >= USB_POLL_INTERVAL_MS) {
            last_poll_ms = now_ms;

            mode_manager_update();
            hid_handler_send_report();
        }
    }

    return 0;
}
