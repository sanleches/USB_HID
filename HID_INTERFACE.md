# HID Interface Reference — Custom Stream Deck

Everything needed to detect, enumerate, read from, and write to this device from userspace or a kernel module.

---

## Device Identification

### USB Descriptors

| Field | Value | Hex |
|-------|-------|-----|
| Vendor ID | `0x1209` | `09 12` |
| Product ID | `0x0001` | `01 00` |
| bcdDevice | `0x0100` | `00 01` |
| bcdUSB | `0x0200` (USB 2.0) | `00 02` |
| bDeviceClass | `0x00` (per-interface) | `00` |
| Manufacturer iString | `"USB HID Project"` | |
| Product iString | `"Custom Stream Deck v1"` | |
| Serial iString | Board unique ID | |

### HID Descriptor

| Field | Value |
|-------|-------|
| Usage Page | `0xFF00` (Vendor-defined) |
| Usage | `0x01` |
| bInterfaceProtocol | `0x00` (None) |
| Endpoint In | `0x81` (EP1, IN) — Interrupt, 64 bytes, polling 10ms |
| Endpoint Out | `0x01` (EP1, OUT) — Interrupt, 64 bytes, polling 10ms |

---

## HID Report Protocol

### Report ID 1 — Input (Device → Host)

Reports button press/release states. Sent over the INTERRUPT IN endpoint.

**Raw USB packet:** `[Report ID] [4 bytes button mask]`

| Offset | Size | Description |
|--------|------|-------------|
| 0 | 1 | Report ID: `0x01` |
| 1 | 1 | Button mask byte 0 — HID indices 0..7 |
| 2 | 1 | Button mask byte 1 — HID indices 8..15 |
| 3 | 1 | Button mask byte 2 — HID indices 16..23 |
| 4 | 1 | Button mask byte 3 — HID indices 24..31 |

**Bit layout (per byte):**

```
Byte 0:  B7  B6  B5  B4  B3  B2  B1  B0
Bit=1 → pressed, Bit=0 → released
```

### Report ID 2 — Output (Host → Device)

Controls LEDs. Sent over the INTERRUPT OUT endpoint. Currently accepted but has no visible effect (reserved for future use).

**Raw USB packet:** `[Report ID] [1 byte LED mask]`

| Offset | Size | Description |
|--------|------|-------------|
| 0 | 1 | Report ID: `0x02` |
| 1 | 1 | LED mask byte — LED indices 0..7 |

---

## Detection Methods

### Linux — `lsusb`

```bash
$ lsusb -d 1209:0001 -v 2>/dev/null | head -40

Bus 001 Device 003: ID 1209:0001 Generic USB HID Project Custom Stream Deck v1
Device Descriptor:
  bcdUSB               2.00
  bDeviceClass          0
  idVendor           0x1209
  idProduct          0x0001
  bcdDevice            1.00
  iManufacturer        1 USB HID Project
  iProduct             2 Custom Stream Deck v1
  iSerial              3 ...
  bNumConfigurations   1
```

### Linux — `hidraw` enumeration

```bash
$ for d in /dev/hidraw*; do
    udevadm info "$d" 2>/dev/null | grep -q "1209:0001" && echo "$d"
  done
/dev/hidraw2
```

### Linux — udev rule (persistent device node)

Create `/etc/udev/rules.d/99-streamdeck.rules`:

```udev
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="1209", ATTRS{idProduct}=="0001", MODE="0666"
SUBSYSTEM=="usb",    ATTRS{idVendor}=="1209", ATTRS{idProduct}=="0001", MODE="0666"
```

Or for the HID subsystem:

```udev
SUBSYSTEM=="hid", ATTRS{idVendor}=="1209", ATTRS{idProduct}=="0001", MODE="0666"
```

---

## Python — Reading Input (hidapi)

```python
#!/usr/bin/env python3
"""Monitor Custom Stream Deck button presses."""

import hid
import sys
import time

VID = 0x1209
PID = 0x0001
REPORT_ID_INPUT = 0x01
INPUT_REPORT_SIZE = 4  # bytes (button mask only, excluding Report ID)

# Human-readable names for HID button indices 0..7
BUTTON_NAMES = ["SW0", "SW1", "SW2", "SW3", "SW4", "SW5", "SW6", "SW7"]


def find_device():
    for dev in hid.enumerate():
        if dev["vendor_id"] == VID and dev["product_id"] == PID:
            return dev
    return None


def main():
    info = find_device()
    if not info:
        print(f"Device {VID:#06x}:{PID:#06x} not found.", file=sys.stderr)
        print("Check USB connection and udev rules.", file=sys.stderr)
        sys.exit(1)

    print(f"Found: {info['manufacturer_string']} {info['product_string']}")
    print(f"  Path: {info['path']}")
    print(f"  Serial: {info['serial_number']}")
    print()

    dev = hid.device()
    dev.open(VID, PID)
    dev.set_nonblocking(True)

    print("Listening for button events...")
    print("Press Ctrl+C to exit.\n")

    prev_mask = 0

    try:
        while True:
            # Read raw report: [ReportID] [4 bytes button mask]
            raw = dev.read(64, timeout_ms=100)
            if not raw:
                continue

            if raw[0] != REPORT_ID_INPUT:
                continue  # skip non-input reports

            # Extract 32-bit button mask
            mask = int.from_bytes(raw[1:5], "little")
            changed = mask ^ prev_mask

            if changed:
                for i in range(len(BUTTON_NAMES)):
                    bit = 1 << i
                    if changed & bit:
                        state = "PRESSED " if mask & bit else "released"
                        print(f"  [{BUTTON_NAMES[i]}] {state}  "
                              f"(mask={mask:#010x})")
                prev_mask = mask

            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\nExiting.")
    finally:
        dev.close()


if __name__ == "__main__":
    main()
```

### Install & run

```bash
pip install hid
sudo ./monitor.py          # first run (udev rule may not be loaded)
# After setting up udev rule:
./monitor.py               # run as normal user
```

---

## Python — Writing Output (LEDs)

```python
#!/usr/bin/env python3
"""Send LED control report to Custom Stream Deck."""

import hid
import sys

VID = 0x1209
PID = 0x0001
REPORT_ID_OUTPUT = 0x02


def set_leds(led_mask):
    dev = hid.device()
    dev.open(VID, PID)
    # Report format: [ReportID=0x02] [LED mask byte]
    report = bytes([REPORT_ID_OUTPUT, led_mask])
    dev.write(report)
    dev.close()


if __name__ == "__main__":
    mask = int(sys.argv[1], 0) if len(sys.argv) > 1 else 0xFF
    print(f"Sending LED mask: {mask:#04x}")
    set_leds(mask)
```

Usage:

```bash
python3 led_control.py 0x0F   # turn on LEDs 0-3
python3 led_control.py 0      # all off
```

---

## C — Kernel Module Driver Skeleton

```c
// SPDX-License-Identifier: GPL-2.0-only
/*
 * Custom Stream Deck — kernel HID driver
 *
 * Matches on VID 0x1209 / PID 0x0001, vendor usage page 0xFF00.
 * Translates HID raw events into input events or forwards to userspace.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hid.h>
#include <linux/usb.h>

#define USB_VID_CUSTOM_STREAMDECK     0x1209
#define USB_PID_CUSTOM_STREAMDECK     0x0001

#define HID_REPORT_ID_INPUT           0x01
#define HID_REPORT_ID_OUTPUT          0x02
#define HID_BUTTON_COUNT              32

/* ── Device ID table ──────────────────────────────────────────── */

static const struct hid_device_id streamdeck_table[] = {
    { HID_USB_DEVICE(USB_VID_CUSTOM_STREAMDECK, USB_PID_CUSTOM_STREAMDECK) },
    { }
};
MODULE_DEVICE_TABLE(hid, streamdeck_table);

/* ── Driver data ──────────────────────────────────────────────── */

struct streamdeck_drvdata {
    struct input_dev *input;
    unsigned long    button_mask;   /* current button state */
};

/* ── Raw event handler ────────────────────────────────────────── */

static int streamdeck_raw_event(struct hid_device *hdev,
                                struct hid_report *report,
                                u8 *data, int size)
{
    struct streamdeck_drvdata *drvdata = hid_get_drvdata(hdev);

    if (!drvdata || !data || size < 5)
        return 0;

    /* data[0] = Report ID */
    if (data[0] != HID_REPORT_ID_INPUT)
        return 0;

    /* data[1..4] = 32-bit button mask (little-endian) */
    u32 mask = get_unaligned_le32(&data[1]);
    u32 changed = mask ^ (u32)drvdata->button_mask;
    int i;

    for_each_set_bit(i, (unsigned long *)&changed, HID_BUTTON_COUNT) {
        bool pressed = mask & BIT(i);
        input_report_key(drvdata->input, BTN_0 + i, pressed);
    }
    input_sync(drvdata->input);

    drvdata->button_mask = mask;
    return 0;
}

/* ── Input device setup ──────────────────────────────────────── */

static int streamdeck_input_setup(struct streamdeck_drvdata *drvdata,
                                  struct hid_device *hdev)
{
    struct input_dev *input;
    int ret, i;

    input = devm_input_allocate_device(&hdev->dev);
    if (!input)
        return -ENOMEM;

    input->name    = "Custom Stream Deck";
    input->phys    = hdev->phys;
    input->uniq    = hdev->uniq;
    input->id.bustype = BUS_USB;
    input->id.vendor  = USB_VID_CUSTOM_STREAMDECK;
    input->id.product = USB_PID_CUSTOM_STREAMDECK;
    input->dev.parent = &hdev->dev;

    /* Register buttons BTN_0 .. BTN_31 */
    __set_bit(EV_KEY, input->evbit);
    for (i = 0; i < HID_BUTTON_COUNT; i++)
        __set_bit(BTN_0 + i, input->keybit);

    ret = input_register_device(input);
    if (ret)
        return ret;

    drvdata->input = input;
    return 0;
}

/* ── Probe / Remove ───────────────────────────────────────────── */

static int streamdeck_probe(struct hid_device *hdev,
                            const struct hid_device_id *id)
{
    struct streamdeck_drvdata *drvdata;
    int ret;

    drvdata = devm_kzalloc(&hdev->dev, sizeof(*drvdata), GFP_KERNEL);
    if (!drvdata)
        return -ENOMEM;

    hid_set_drvdata(hdev, drvdata);

    ret = hid_parse(hdev);
    if (ret)
        return ret;

    ret = hid_hw_start(hdev, HID_CONNECT_HIDRAW);
    if (ret)
        return ret;

    ret = streamdeck_input_setup(drvdata, hdev);
    if (ret) {
        hid_hw_stop(hdev);
        return ret;
    }

    hid_info(hdev, "Custom Stream Deck v1 registered\n");
    return 0;
}

static void streamdeck_remove(struct hid_device *hdev)
{
    hid_hw_stop(hdev);
}

/* ── Driver registration ─────────────────────────────────────── */

static struct hid_driver streamdeck_driver = {
    .name       = "custom-streamdeck",
    .id_table   = streamdeck_table,
    .probe      = streamdeck_probe,
    .remove     = streamdeck_remove,
    .raw_event  = streamdeck_raw_event,
};

module_hid_driver(streamdeck_driver);

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("HID driver for Custom Stream Deck (VID 1209 / PID 0001)");
MODULE_LICENSE("GPL");
```

### Build & test the kernel module

```makefile
# Makefile (place alongside streamdeck.c)
obj-m += streamdeck.o

KDIR := /lib/modules/$(shell uname -r)/build

all:
    $(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
    $(MAKE) -C $(KDIR) M=$(PWD) clean

install:
    sudo insmod streamdeck.ko
    sudo udevadm trigger

uninstall:
    sudo rmmod streamdeck
```

```bash
make
sudo insmod streamdeck.ko
# Check: dmesg | tail
# Check: /proc/bus/input/devices | grep -A 5 Stream
```

---

## libusb — Low-Level Access (without HID driver) 

If the kernel HID driver claims the device, unbind it first:

```bash
# Find the device
lsusb -d 1209:0001
# Bus=001 Device=003

# Unbind from HID driver
echo -n "1-3:1.0" | sudo tee /sys/bus/usb/drivers/usbhid/unbind
```

Then use libusb directly in C or via Python's `pyusb`:

```python
import usb.core
import usb.util

dev = usb.core.find(idVendor=0x1209, idProduct=0x0001)
if dev is None:
    raise ValueError("Device not found")

if dev.is_kernel_driver_active(0):
    dev.detach_kernel_driver(0)

cfg = dev.get_active_configuration()
intf = cfg[(0, 0)]

ep_in  = usb.util.find_descriptor(intf, custom_match=lambda e:
          usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_IN)
ep_out = usb.util.find_descriptor(intf, custom_match=lambda e:
          usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_OUT)

# Read input report
data = dev.read(ep_in.bEndpointAddress, 64, timeout=1000)
# data[0] = Report ID (0x01), data[1:5] = button mask

# Write LED output report
dev.write(ep_out.bEndpointAddress, [0x02, 0xFF], timeout=1000)
```

---

## Summary Cheatsheet

```
┌─────────────────────────────────────────────────────────────┐
│  VID=0x1209  PID=0x0001  UsagePage=0xFF00  Usage=0x01      │
├─────────────────────────────────────────────────────────────┤
│  IN  EP 0x81  │  Report ID 1  │  4 bytes  →  32 buttons   │
│  OUT EP 0x01  │  Report ID 2  │  1 byte   →  8 LEDs       │
├─────────────────────────────────────────────────────────────┤
│  hidraw: /dev/hidraw*  │  /sys matches on idVendor/idProduct│
│  udev: ATTRS{idVendor}=="1209", ATTRS{idProduct}=="0001"   │
└─────────────────────────────────────────────────────────────┘
```
