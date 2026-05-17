# Usage Guide

## Building

### Prerequisites

- Raspberry Pi Pico SDK 2.2.0 (installed at `~/.pico-sdk/sdk/2.2.0`)
- ARM GCC toolchain (14.2.Rel1)
- CMake 3.13+, Ninja build system

### Build Steps

```bash
cd Usb_hid/build
cmake ..
ninja
```

Output files in `build/`:
- `Usb_hid.uf2` — Flashable firmware
- `Usb_hid.elf` — Debug symbols
- `Usb_hid.hex` — Intel HEX format
- `Usb_hid.bin` — Raw binary

---

## Flashing

### Method 1: picotool (if already running)

```bash
picotool load build/Usb_hid.uf2 -f
picotool reboot
```

### Method 2: UF2 drag-and-drop

1. Hold the **BOOTSEL** button on the Pico
2. Connect USB to your computer
3. Release BOOTSEL — the Pico appears as a USB mass storage device
4. Copy `build/Usb_hid.uf2` to the drive
5. The Pico automatically reboots and runs the firmware

---

## Hardware Setup

### Demo Mode (no wiring required)

Leave GPIO 14 unconnected (internal pull-down pulls it LOW). The device auto-cycles buttons 0–3 at 1-second intervals. Plug in USB and observe.

### Normal Mode (with physical buttons)

#### Required components (per button):
- 1x momentary push button (NO — normally open)
- 1x wire

#### Wiring:

```
Pico 2 W                   Buttons
─────────                  ───────
GPIO 2  ──────┬──────┤ SW0 ├──── GND
GPIO 3  ──────┬──────┤ SW1 ├──── GND
GPIO 4  ──────┬──────┤ SW2 ├──── GND
GPIO 5  ──────┬──────┤ SW3 ├──── GND
GPIO 6  ──────┬──────┤ SW4 ├──── GND
GPIO 7  ──────┬──────┤ SW5 ├──── GND
GPIO 8  ──────┬──────┤ SW6 ├──── GND
GPIO 9  ──────┬──────┤ SW7 ├──── GND

GPIO 14 ──────┬──────┤ Mode ├──── +3.3V  (HIGH = Normal)
              │      Switch│
              └────────────┘
```

No external resistors needed — the firmware enables the Pico's internal pull-up on all button GPIOs.

#### Mode selection:
- **GPIO 14 → 3.3V** (HIGH) = Normal mode (read physical buttons)
- **GPIO 14 → floating** (LOW) = Demo mode (auto-cycle)

---

## Debug Output

Connect a USB-UART adapter to GPIO 0 (TX) and GPIO 1 (RX) at **115200 baud**.

On startup, the firmware prints:
```
Custom Stream Deck v1
Mode: DEMO
VID: 0x1209  PID: 0x0001
```

(or `Mode: NORMAL` if GPIO 14 is pulled HIGH).

---

## Status LED (Pico W only)

The on-board LED indicates USB status:

| Blink Pattern | Meaning |
|:-------------:|---------|
| Fast (100ms) | USB not mounted / starting up |
| Slow (1000ms) | USB mounted, running |
| Very slow (2500ms) | USB suspended |

Note: the LED only works on Pico W / Pico 2 W (requires CYW43 chip).

---

## Testing on the Host

### Linux — verify device is recognized

```bash
# Check USB device
lsusb | grep 1209

# Check HID device
ls /dev/hidraw*

# Read raw HID reports (requires root)
sudo cat /dev/hidraw0 | xxd
```

### Linux — using hidapi to monitor reports

```python
import hid

device = hid.device()
device.open(0x1209, 0x0001)
device.set_nonblocking(1)

while True:
    report = device.read(64)
    if report:
        buttons = int.from_bytes(report[1:5], 'little')
        print(f"Buttons: {buttons:032b}")
```

### Expected Demo Mode output

In demo mode, the report cycles through:
- Only button 0 pressed: `...0001`
- Only button 1 pressed: `...0010`
- Only button 2 pressed: `...0100`
- Only button 3 pressed: `...1000`
- Repeat every 4 seconds

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Device not detected by `lsusb` | USB init issue | Re-flash firmware, check BOOTSEL mode |
| No debug output on UART | Wrong baud rate or wiring | Check UART adapter is 3.3V, set terminal to 115200 8N1 |
| Normal mode not reading buttons | Mode select GPIO floating | Connect GPIO 14 to 3.3V |
| LED not blinking | Non-W variant or CYW43 init failed | Normal on Pico (non-W), connect external LED |
| Buttons not registering | Wiring or debounce | Check pull-down is active, press for >25ms |
