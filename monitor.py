#!/usr/bin/env python3
"""Monitor Custom HID Device button presses.

This is a testing script meant to be run on a machine with linux.
Reads the HID imputs from a flashed Raspberry pi Pico 2 W, acting as a USB HID device.



Usage:
    sudo python3 monitor.py

"""

import sys

sys.path = [p for p in sys.path if not p.endswith("Usb_hid/hid")]

import hid

#Set the VID and PID to match your Vendor and Product id as set on /cofig/hid_config.h
VID = 0x1209
PID = 0x0001

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
        print("Check: lsusb | grep " + hex(VID)[2:], file=sys.stderr)
        sys.exit(1)

    print(f"Found: {info['manufacturer_string']} {info['product_string']}", flush=True)
    print(f"  Path: {info['path']}", flush=True)
    print(f"  UsagePage: 0x{info['usage_page']:04X}", flush=True)
    print(flush=True)

    try:
        dev = hid.Device(VID, PID)
    except hid.HIDException as e:
        print(f"Failed to open device: {e}", file=sys.stderr)
        sys.exit(1)

    prev_mask = 0
    first = True

    print("Listening... (Ctrl+C to exit)\n", flush=True)

    try:
        while True:
            raw = dev.read(64)
            if not raw or raw[0] != 0x01:
                continue

            mask = int.from_bytes(raw[1:5], "little")

            if first:
                prev_mask = mask
                first = False
                print(f"  Initial state: {mask:#010x}")
                continue

            changed = mask ^ prev_mask
            if changed:
                for i, name in enumerate(BUTTON_NAMES):
                    bit = 1 << i
                    if changed & bit:
                        state = "PRESSED" if mask & bit else "released"
                        print(f"  {name} {state}")
                prev_mask = mask

    except KeyboardInterrupt:
        print("\nExiting.")
    finally:
        dev.close()


if __name__ == "__main__":
    main()
