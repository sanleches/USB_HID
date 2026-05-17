#!/usr/bin/env python3
"""Monitor USB HID button reports from the Pico firmware."""

import sys
import time
from dataclasses import dataclass
from pathlib import Path

# Prevent importing this repository's local `hid/` C source folder as a
# namespace package; we need the third-party Python `hid` module instead.
PROJECT_DIR = Path(__file__).resolve().parent
sys.path = [p for p in sys.path if Path(p or ".").resolve() != PROJECT_DIR]

try:
    import hid
except ModuleNotFoundError:
    print(
        "Missing Python HID module. Install with: sudo dnf install python3-hidapi",
        file=sys.stderr,
    )
    sys.exit(1)

VID = 0x1209
PID = 0x0001
BUTTON_NAMES = ["SW0", "SW1", "SW2", "SW3", "SW4", "SW5", "SW6", "SW7"]


@dataclass
class MonitorOptions:
    mode: str
    ui_fps: float = 8.0
    status_sec: float = 1.0


@dataclass
class MonitorState:
    current_mask: int = 0
    prev_mask: int = 0
    first_report: bool = True
    report_count: int = 0
    started_at: float = 0.0
    last_report_at: float = 0.0
    last_change_at: float = 0.0
    last_status_at: float = 0.0
    last_event: str = "waiting for first report"
    last_raw: str = "n/a"


def find_device():
    for dev in hid.enumerate():
        if dev["vendor_id"] == VID and dev["product_id"] == PID:
            return dev
    return None


def open_device(vendor_id, product_id):
    if hasattr(hid, "Device"):
        return hid.Device(vendor_id, product_id)

    if hasattr(hid, "device"):
        dev = hid.device()
        dev.open(vendor_id, product_id)
        return dev

    raise RuntimeError("Unsupported hid module: missing Device/device API")


def extract_mask(raw_report):
    if len(raw_report) >= 5 and raw_report[0] == 0x01:
        return int.from_bytes(raw_report[1:5], "little")
    if len(raw_report) >= 4:
        return int.from_bytes(raw_report[0:4], "little")
    return None


def format_pressed(mask):
    pressed = [name for i, name in enumerate(BUTTON_NAMES) if mask & (1 << i)]
    return ", ".join(pressed) if pressed else "none"


def mask_bits(mask, width=8):
    return "".join("1" if mask & (1 << i) else "0" for i in range(width - 1, -1, -1))


def ask_choice(prompt, valid_choices, default):
    while True:
        value = input(prompt).strip().lower()
        if not value:
            return default
        if value in valid_choices:
            return value
        print(f"Invalid choice. Valid options: {', '.join(valid_choices)}")


def ask_float(prompt, default, minimum):
    while True:
        value = input(prompt).strip()
        if not value:
            return default
        try:
            parsed = float(value)
        except ValueError:
            print("Please type a number.")
            continue
        if parsed <= minimum:
            print(f"Value must be > {minimum}.")
            continue
        return parsed


def choose_options():
    print("USB HID Monitor")
    print("=============")
    print("1) Live dashboard (recommended)")
    print("2) Event log")
    print("3) Raw report dump")
    print("q) Quit")

    choice = ask_choice("Select mode [1]: ", {"1", "2", "3", "q"}, "1")
    if choice == "q":
        sys.exit(0)

    if choice == "1":
        fps = ask_float("Refresh rate FPS [8]: ", 8.0, 0)
        return MonitorOptions(mode="ui", ui_fps=fps)
    if choice == "2":
        status = ask_float("Status interval seconds [1.0]: ", 1.0, 0)
        return MonitorOptions(mode="events", status_sec=status)
    return MonitorOptions(mode="dump")


def init_state():
    now = time.monotonic()
    return MonitorState(
        started_at=now,
        last_report_at=now,
        last_change_at=now,
        last_status_at=now,
    )


def apply_report(state, raw, now):
    state.report_count += 1
    state.last_raw = " ".join(f"{b:02X}" for b in raw[:8])

    mask = extract_mask(raw)
    if mask is None:
        return []

    events = []
    state.current_mask = mask
    state.last_report_at = now

    if state.first_report:
        state.first_report = False
        state.prev_mask = mask
        state.last_change_at = now
        state.last_event = f"initial state {mask:#010x}"
        events.append(f"Initial state: {mask:#010x}")
        return events

    changed = mask ^ state.prev_mask
    if changed:
        state.last_change_at = now
        state.last_event = f"mask {state.prev_mask:#010x} -> {mask:#010x}"
        events.append(f"State: {state.prev_mask:#010x} -> {mask:#010x}")
        for i, name in enumerate(BUTTON_NAMES):
            bit = 1 << i
            if changed & bit:
                pressed = bool(mask & bit)
                state.last_event = f"{name} {'PRESSED' if pressed else 'released'}"
                events.append(f"{name} {'PRESSED' if pressed else 'released'}")

    state.prev_mask = mask
    return events


def render_dashboard(info, state):
    now = time.monotonic()
    age_ms = int((now - state.last_report_at) * 1000)
    unchanged_ms = int((now - state.last_change_at) * 1000)
    uptime_s = now - state.started_at

    lines = [
        "USB HID Monitor (Ctrl+C to exit)",
        "",
        f"Device: {info.get('manufacturer_string', '')} {info.get('product_string', '')}",
        f"VID:PID 0x{VID:04x}:0x{PID:04x}",
        f"Path:   {info.get('path', '')}",
        "",
        f"Mask:   {state.current_mask:#010x}  bits[{mask_bits(state.current_mask)}]",
        f"Pressed:{format_pressed(state.current_mask)}",
        f"Reports:{state.report_count}  last report:{age_ms} ms ago",
        f"State unchanged: {unchanged_ms} ms",
        f"Last event: {state.last_event}",
        f"Last raw:   {state.last_raw}",
        f"Uptime: {uptime_s:0.1f}s",
        "",
        "Buttons:",
    ]

    for i, name in enumerate(BUTTON_NAMES):
        label = "PRESSED " if state.current_mask & (1 << i) else "released"
        lines.append(f"  {name}: {label}")

    lines.extend([
        "",
        "Tip: if a button stays PRESSED, wiring is likely held low.",
    ])
    return lines


def paint_fullscreen(lines):
    sys.stdout.write("\x1b[H")
    for line in lines:
        sys.stdout.write("\x1b[2K")
        sys.stdout.write(line)
        sys.stdout.write("\n")
    sys.stdout.write("\x1b[J")
    sys.stdout.flush()


def run_ui_loop(dev, info, options):
    state = init_state()
    frame_period = 1.0 / options.ui_fps
    next_frame = time.monotonic()

    print("\nListening... (Ctrl+C to exit)")
    sys.stdout.write("\x1b[2J\x1b[H")
    sys.stdout.flush()

    try:
        while True:
            now = time.monotonic()
            raw = dev.read(64)
            if raw:
                apply_report(state, raw, now)

            if now >= next_frame:
                paint_fullscreen(render_dashboard(info, state))
                next_frame = now + frame_period

            if not raw:
                time.sleep(0.005)
    except KeyboardInterrupt:
        sys.stdout.write("\x1b[2J\x1b[H")
        sys.stdout.flush()
        print("Exiting.")


def run_event_loop(dev, options):
    state = init_state()
    print("\nListening... (Ctrl+C to exit)\n")

    try:
        while True:
            now = time.monotonic()
            raw = dev.read(64)
            if raw:
                for event in apply_report(state, raw, now):
                    print(f"  {event}")

            if now - state.last_status_at >= options.status_sec:
                age_ms = int((now - state.last_report_at) * 1000)
                print(
                    f"  status: mask={state.current_mask:#010x} "
                    f"pressed=[{format_pressed(state.current_mask)}] "
                    f"reports={state.report_count} last_report={age_ms}ms ago"
                )
                state.last_status_at = now

            if not raw:
                time.sleep(0.005)
    except KeyboardInterrupt:
        print("\nExiting.")


def run_dump_loop(dev):
    state = init_state()
    print("\nListening... (Ctrl+C to exit)\n")

    try:
        while True:
            now = time.monotonic()
            raw = dev.read(64)
            if raw:
                events = apply_report(state, raw, now)
                print(f"  report[{state.report_count}] raw={state.last_raw} mask={state.current_mask:#010x}")
                for event in events:
                    print(f"    {event}")
            else:
                time.sleep(0.005)
    except KeyboardInterrupt:
        print("\nExiting.")


def main():
    options = choose_options()

    info = find_device()
    if not info:
        print(f"Device {VID:#06x}:{PID:#06x} not found.", file=sys.stderr)
        print("Check: lsusb | grep " + hex(VID)[2:], file=sys.stderr)
        sys.exit(1)

    print(f"\nFound: {info.get('manufacturer_string', '')} {info.get('product_string', '')}")
    print(f"  Path: {info.get('path', '')}")
    print(f"  UsagePage: 0x{info.get('usage_page', 0):04X}")

    try:
        dev = open_device(VID, PID)
    except Exception as err:
        print(f"Failed to open device: {err}", file=sys.stderr)
        sys.exit(1)

    try:
        if hasattr(dev, "set_nonblocking"):
            dev.set_nonblocking(True)

        if options.mode == "ui":
            run_ui_loop(dev, info, options)
        elif options.mode == "events":
            run_event_loop(dev, options)
        else:
            run_dump_loop(dev)
    finally:
        dev.close()


if __name__ == "__main__":
    main()
