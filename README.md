# Custom Stream Deck — USB HID Button Controller

A Raspberry Pi Pico 2 W firmware that emulates a Stream Deck-like USB HID device with 32 virtual buttons. Built on Pico SDK 2.2.0 + TinyUSB 0.18.0.

## Features

- **USB HID vendor-defined device** (Usage Page 0xFF00) — 32 buttons IN, 8 LEDs OUT
- **Two modes**: Demo (auto-cycle) and Normal (physical GPIO buttons)
- **Debounced GPIO reading** with internal pull-down resistors
- **Proper device identification** for kernel module matching (VID 0x1209 / PID 0x0001)
- **Status LED** via Pico W on-board LED (CYW43)

## Quick Start

```bash
# Build
cd build && cmake .. && ninja

# Flash (hold BOOTSEL, connect USB)
picotool load build/Usb_hid.uf2 -f
# or copy Usb_hid.uf2 to the Pico's USB mass storage drive
```

## Documentation

| File | Content |
|------|---------|
| `TECHNICAL.md` | Architecture, modules, HID protocol, USB descriptors, kernel module matching |
| `USAGE.md` | Wiring, mode selection, hardware setup, testing |

## Project Structure

```
Usb_hid/
├── CMakeLists.txt          # Build system
├── main.c                  # Entry point
├── config/hid_config.h     # All tunable parameters
├── usb/
│   ├── tusb_config.h       # TinyUSB stack config
│   ├── usb_descriptors.c   # USB device, HID, string descriptors
│   └── usb_descriptors.h
├── hid/
│   ├── hid_handler.c       # HID report building + TinyUSB callbacks
│   └── hid_handler.h
├── board/
│   ├── button.c            # GPIO init, debounce read
│   └── button.h
└── modes/
    ├── mode_manager.c      # Mode detection + dispatch
    ├── mode_manager.h
    ├── demo_mode.c         # Auto-cycle 4 buttons
    ├── demo_mode.h
    ├── normal_mode.c       # Physical button reading
    └── normal_mode.h
```

## License

MIT
