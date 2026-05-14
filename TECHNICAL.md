# Technical Documentation

## Architecture

```
main.c
  ├── board_init()              — Pico SDK + UART init
  ├── cyw43_arch_init()         — CYW43 WiFi chip (for LED)
  ├── hid_handler_init()        — Clear button state
  ├── tusb_init()               — Start TinyUSB device stack
  ├── mode_manager_init()       — Read mode GPIO → init module
  └── main loop
       ├── tud_task()           — USB event processing
       ├── led_blinking_task()  — Status LED (mounted/unmounted)
       ├── mode_manager_update()-> delegates to demo_mode or normal_mode
       └── hid_handler_send_report() — Send HID report if dirty
```

### Module Dependencies

```
main.c
  ├── config/hid_config.h        (no deps)
  ├── usb/tusb_config.h          (no deps)
  ├── usb/usb_descriptors.c      → config/hid_config.h, tusb.h
  ├── hid/hid_handler.c          → config/hid_config.h, tusb.h
  ├── board/button.c             → config/hid_config.h, hardware/gpio.h
  ├── modes/demo_mode.c          → config/hid_config.h, hid/hid_handler.h
  ├── modes/normal_mode.c        → config/hid_config.h, hid/hid_handler.h, board/button.h
  └── modes/mode_manager.c       → config/hid_config.h, demo_mode.h, normal_mode.h
```

---

## USB HID Protocol

### Device Identification

| Field | Value |
|-------|-------|
| Vendor ID | `0x1209` (Generic USB PID range) |
| Product ID | `0x0001` |
| bcdDevice | `0x0100` |
| Manufacturer String | `"USB HID Project"` |
| Product String | `"Custom Stream Deck v1"` |
| Serial | Board unique ID (via `board_get_unique_id`) |

**Kernel module matching:** Match on `VID 0x1209 + PID 0x0001` or HID usage page `0xFF00`.

### HID Report Descriptor

```
Usage Page (Vendor Page: 0xFF00)
Usage (0x01 — Stream Deck)
Collection (Application)
  Report ID (1)
  Logical Minimum (0)
  Logical Maximum (1)
  Report Size (1)         // 1 bit per button
  Report Count (32)       // 32 buttons
  Usage (0x01 — Buttons)
  Input (Data, Var, Abs)  // IN endpoint (device → host)

  Report ID (2)
  Logical Minimum (0)
  Logical Maximum (1)
  Report Size (1)         // 1 bit per LED
  Report Count (8)        // 8 LEDs
  Usage (0x02 — LEDs)
  Output (Data, Var, Abs) // OUT endpoint (host → device)
End Collection
```

### Report Formats

**Input Report (Report ID 1)** — 5 bytes total over USB:

| Byte | Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
|------|-------|-------|-------|-------|-------|-------|-------|-------|
| 0    | B7    | B6    | B5    | B4    | B3    | B2    | B1    | B0    |
| 1    | B15   | B14   | B13   | B12   | B11   | B10   | B9    | B8    |
| 2    | B23   | B22   | B21   | B20   | B19   | B18   | B17   | B16   |
| 3    | B31   | B30   | B29   | B28   | B27   | B26   | B25   | B24   |

Bit = 1 → pressed, Bit = 0 → released.

**Output Report (Report ID 2)** — 2 bytes total over USB:

| Byte | Bit 7-1 | Bit 0 |
|------|---------|-------|
| 0    | (reserved) | LED states 7..0 (future use) |

---

## Pin Mapping (Normal Mode)

| HID Button Index | GPIO Pin | Pull |
|-----------------:|:--------:|:----:|
| 0                | 2        | Down |
| 1                | 3        | Down |
| 2                | 4        | Down |
| 3                | 5        | Down |
| 4                | 6        | Down |
| 5                | 7        | Down |
| 6                | 8        | Down |
| 7                | 9        | Down |

### Other Pins

| Pin  | Function |
|:----:|----------|
| GPIO 0 | UART TX (debug, 115200 baud) |
| GPIO 1 | UART RX (debug) |
| GPIO 14 | Mode select (LOW=Demo, HIGH=Normal, pulled down) |
| USB D+ | Pico built-in USB |
| USB D- | Pico built-in USB |

Button wiring: momentary NO switch between GPIO pin and 3.3V. Internal pull-down holds the pin LOW when not pressed.

---

## Mode Manager

Mode is read once at startup from GPIO 14:

| GPIO 14 | Mode | Behavior |
|:-------:|:----:|----------|
| LOW (default) | `DEMO` | Auto-cycles through buttons 0–3. Each button held 1000ms. No hardware needed. |
| HIGH | `NORMAL` | Reads buttons 0–7 from GPIO 2–9 with debounce. |

### Debounce

- Window: 25ms (`DEBOUNCE_MS` in `config/hid_config.h`)
- Algorithm: Button state must be stable for the full debounce window before being accepted.
- Polling: Every 10ms (`USB_POLL_INTERVAL_MS`).

---

## Configuration

All tunable parameters are in `config/hid_config.h`:

| Macro | Default | Description |
|-------|---------|-------------|
| `USB_VID` | `0x1209` | USB Vendor ID |
| `USB_PID` | `0x0001` | USB Product ID |
| `HID_INPUT_BUTTON_COUNT` | `32` | Number of virtual buttons |
| `HID_INPUT_REPORT_SIZE` | `4` | Input report bytes |
| `MODE_SELECT_GPIO` | `14` | Mode selection pin |
| `DEMO_BUTTON_INDICES` | `{0,1,2,3}` | Buttons to cycle in demo |
| `DEMO_PRESS_INTERVAL_MS` | `1000` | Demo hold time per button |
| `NORMAL_BUTTON_COUNT` | `8` | Physical button count |
| `NORMAL_BUTTON_GPIO_PINS` | `{2,3,4,5,6,7,8,9}` | Physical button GPIOs |
| `DEBOUNCE_MS` | `25` | Debounce window |
| `USB_POLL_INTERVAL_MS` | `10` | Main loop poll rate |

---

## Writing a Kernel Module Driver

The device can be identified in a Linux kernel module by:

```c
// USB device match
static const struct usb_device_id streamdeck_table[] = {
    { USB_DEVICE(0x1209, 0x0001) },
    { }
};
MODULE_DEVICE_TABLE(usb, streamdeck_table);

// Or via HID driver match
static const struct hid_device_id streamdeck_hid_table[] = {
    { HID_USB_DEVICE(0x1209, 0x0001) },
    { }
};
MODULE_DEVICE_TABLE(hid, streamdeck_hid_table);
```

The HID report uses vendor-defined usage page `0xFF00`. Raw event data can be read from the HID device node (`/dev/hidraw*`) or via the HID subsystem's `->raw_event` or `->input_event` callbacks.
