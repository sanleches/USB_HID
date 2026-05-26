#!/usr/bin/env python3
"""
=============================================================================
USB HID Button Monitor
=============================================================================

Host-side diagnostic tool for the Raspberry Pi Pico USB HID firmware.

The firmware sends a vendor-defined HID input report containing a 32-bit button
bitmask. This script opens that HID device with hidapi, decodes the incoming
button reports, and presents them in one of three formats:

  1. Live dashboard - full-screen terminal UI for wiring/debug sessions
  2. Event log      - one line per state transition plus periodic status
  3. Raw dump       - raw report bytes plus decoded mask

Keep DEVICE_VENDOR_ID and DEVICE_PRODUCT_ID in sync with config/hid_config.h.
"""

from __future__ import annotations

import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Sequence


# -----------------------------------------------------------------------------
# Third-Party HID Module Loading
# -----------------------------------------------------------------------------

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


# -----------------------------------------------------------------------------
# Configuration
# -----------------------------------------------------------------------------

DEVICE_VENDOR_ID = 0x6666
DEVICE_PRODUCT_ID = 0x0001
INPUT_REPORT_ID = 0x01

HID_READ_SIZE_BYTES = 64
RAW_PREVIEW_BYTES = 8
IDLE_SLEEP_SEC = 0.005

DEFAULT_UI_FPS = 8.0
DEFAULT_STATUS_SEC = 1.0

# The firmware supports a 32-bit button mask. The current physical wiring maps
# the first eight bits to SW0..SW7, so the UI highlights those named buttons.
BUTTON_NAMES = ["SW0", "SW1", "SW2", "SW3", "SW4", "SW5", "SW6", "SW7"]


@dataclass
class MonitorOptions:
    """User-selected monitor mode and refresh timing."""

    mode: str
    ui_fps: float = DEFAULT_UI_FPS
    status_sec: float = DEFAULT_STATUS_SEC


@dataclass
class MonitorState:
    """Decoded HID state shared by all monitor modes."""

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


# -----------------------------------------------------------------------------
# HID Device Helpers
# -----------------------------------------------------------------------------

def find_device() -> dict[str, Any] | None:
    """Return hidapi metadata for the configured device, if connected."""

    for dev in hid.enumerate():
        if (
            dev["vendor_id"] == DEVICE_VENDOR_ID
            and dev["product_id"] == DEVICE_PRODUCT_ID
        ):
            return dev
    return None


def open_device(vendor_id: int, product_id: int) -> Any:
    """
    Open a HID device across the two common Python hidapi bindings.

    Fedora's python3-hidapi exposes `hid.Device`; older hidapi wrappers expose
    `hid.device()`. Supporting both keeps this monitor easy to run on different
    Linux distributions without changing the rest of the code.
    """

    if hasattr(hid, "Device"):
        return hid.Device(vendor_id, product_id)

    if hasattr(hid, "device"):
        dev = hid.device()
        dev.open(vendor_id, product_id)
        return dev

    raise RuntimeError("Unsupported hid module: missing Device/device API")


def configure_nonblocking(dev: Any) -> None:
    """Enable non-blocking reads when the selected hidapi binding supports it."""

    if hasattr(dev, "set_nonblocking"):
        dev.set_nonblocking(True)


# -----------------------------------------------------------------------------
# HID Report Decoding and Formatting
# -----------------------------------------------------------------------------

def extract_mask(raw_report: Sequence[int]) -> int | None:
    """
    Decode the 32-bit little-endian button mask from an input report.

    Depending on platform and hidapi binding, interrupt reads may either include
    the report ID as byte 0 or return only the report payload. Accept both forms
    so the monitor works with the same firmware across host environments.
    """

    if len(raw_report) >= 5 and raw_report[0] == INPUT_REPORT_ID:
        return int.from_bytes(bytes(raw_report[1:5]), "little")
    if len(raw_report) >= 4:
        return int.from_bytes(bytes(raw_report[0:4]), "little")
    return None


def format_pressed(mask: int) -> str:
    """Return a comma-separated list of named buttons currently pressed."""

    pressed = [name for i, name in enumerate(BUTTON_NAMES) if mask & (1 << i)]
    return ", ".join(pressed) if pressed else "none"


def format_mask_bits(mask: int, width: int = len(BUTTON_NAMES)) -> str:
    """Format the low `width` bits with the most-significant bit first."""

    return "".join(
        "1" if mask & (1 << i) else "0"
        for i in range(width - 1, -1, -1)
    )


def format_raw_report(raw_report: Sequence[int]) -> str:
    """Show the first few bytes of a HID report as uppercase hex pairs."""

    return " ".join(f"{byte:02X}" for byte in raw_report[:RAW_PREVIEW_BYTES])


# -----------------------------------------------------------------------------
# Interactive Prompt Helpers
# -----------------------------------------------------------------------------

def ask_choice(prompt: str, valid_choices: set[str], default: str) -> str:
    """Prompt until the user enters one of the accepted values."""

    while True:
        value = input(prompt).strip().lower()
        if not value:
            return default
        if value in valid_choices:
            return value
        print(f"Invalid choice. Valid options: {', '.join(sorted(valid_choices))}")


def ask_float(prompt: str, default: float, minimum: float) -> float:
    """Prompt for a floating-point value greater than `minimum`."""

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


def choose_options() -> MonitorOptions:
    """Collect the monitor mode and refresh settings before opening USB."""

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
        fps = ask_float("Refresh rate FPS [8]: ", DEFAULT_UI_FPS, 0)
        return MonitorOptions(mode="ui", ui_fps=fps)
    if choice == "2":
        status = ask_float("Status interval seconds [1.0]: ", DEFAULT_STATUS_SEC, 0)
        return MonitorOptions(mode="events", status_sec=status)
    return MonitorOptions(mode="dump")


# -----------------------------------------------------------------------------
# Monitor State Updates
# -----------------------------------------------------------------------------

def init_state() -> MonitorState:
    """Create a state object with all timing fields initialized to now."""

    now = time.monotonic()
    return MonitorState(
        started_at=now,
        last_report_at=now,
        last_change_at=now,
        last_status_at=now,
    )


def apply_report(
    state: MonitorState,
    raw_report: Sequence[int],
    now: float,
) -> list[str]:
    """
    Decode a raw HID report and return human-readable state-change events.

    The dashboard reads state directly from `MonitorState`; log modes use the
    returned events to print only meaningful transitions.
    """

    state.report_count += 1
    state.last_raw = format_raw_report(raw_report)

    mask = extract_mask(raw_report)
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


# -----------------------------------------------------------------------------
# Terminal Rendering
# -----------------------------------------------------------------------------

def render_dashboard(info: dict[str, Any], state: MonitorState) -> list[str]:
    """Build the full-screen dashboard as a list of terminal lines."""

    now = time.monotonic()
    age_ms = int((now - state.last_report_at) * 1000)
    unchanged_ms = int((now - state.last_change_at) * 1000)
    uptime_s = now - state.started_at
    device_name = (
        f"{info.get('manufacturer_string', '')} "
        f"{info.get('product_string', '')}"
    )

    lines = [
        "USB HID Monitor (Ctrl+C to exit)",
        "",
        f"Device: {device_name}",
        f"VID:PID 0x{DEVICE_VENDOR_ID:04x}:0x{DEVICE_PRODUCT_ID:04x}",
        f"Path:   {info.get('path', '')}",
        "",
        (
            f"Mask:   {state.current_mask:#010x}  "
            f"bits[{format_mask_bits(state.current_mask)}]"
        ),
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


def paint_fullscreen(lines: Sequence[str]) -> None:
    """Paint a stable full-screen terminal view using ANSI escape sequences."""

    sys.stdout.write("\x1b[H")
    for line in lines:
        sys.stdout.write("\x1b[2K")
        sys.stdout.write(line)
        sys.stdout.write("\n")
    sys.stdout.write("\x1b[J")
    sys.stdout.flush()


# -----------------------------------------------------------------------------
# Monitor Modes
# -----------------------------------------------------------------------------

def read_report(dev: Any) -> Sequence[int]:
    """Read one HID report. In non-blocking mode this may return an empty list."""

    return dev.read(HID_READ_SIZE_BYTES)


def sleep_if_idle(raw_report: Sequence[int]) -> None:
    """Avoid a busy loop when non-blocking HID reads have no data available."""

    if not raw_report:
        time.sleep(IDLE_SLEEP_SEC)


def run_ui_loop(dev: Any, info: dict[str, Any], options: MonitorOptions) -> None:
    """Run the live terminal dashboard."""

    state = init_state()
    frame_period = 1.0 / options.ui_fps
    next_frame = time.monotonic()

    print("\nListening... (Ctrl+C to exit)")
    sys.stdout.write("\x1b[2J\x1b[H")
    sys.stdout.flush()

    try:
        while True:
            now = time.monotonic()
            raw_report = read_report(dev)
            if raw_report:
                apply_report(state, raw_report, now)

            if now >= next_frame:
                paint_fullscreen(render_dashboard(info, state))
                next_frame = now + frame_period

            sleep_if_idle(raw_report)
    except KeyboardInterrupt:
        sys.stdout.write("\x1b[2J\x1b[H")
        sys.stdout.flush()
        print("Exiting.")


def run_event_loop(dev: Any, options: MonitorOptions) -> None:
    """Run a compact event log with periodic status lines."""

    state = init_state()
    print("\nListening... (Ctrl+C to exit)\n")

    try:
        while True:
            now = time.monotonic()
            raw_report = read_report(dev)
            if raw_report:
                for event in apply_report(state, raw_report, now):
                    print(f"  {event}")

            if now - state.last_status_at >= options.status_sec:
                age_ms = int((now - state.last_report_at) * 1000)
                print(
                    f"  status: mask={state.current_mask:#010x} "
                    f"pressed=[{format_pressed(state.current_mask)}] "
                    f"reports={state.report_count} last_report={age_ms}ms ago"
                )
                state.last_status_at = now

            sleep_if_idle(raw_report)
    except KeyboardInterrupt:
        print("\nExiting.")


def run_dump_loop(dev: Any) -> None:
    """Print every report exactly as received plus the decoded mask."""

    state = init_state()
    print("\nListening... (Ctrl+C to exit)\n")

    try:
        while True:
            now = time.monotonic()
            raw_report = read_report(dev)
            if raw_report:
                events = apply_report(state, raw_report, now)
                print(
                    f"  report[{state.report_count}] "
                    f"raw={state.last_raw} mask={state.current_mask:#010x}"
                )
                for event in events:
                    print(f"    {event}")
            else:
                sleep_if_idle(raw_report)
    except KeyboardInterrupt:
        print("\nExiting.")


# -----------------------------------------------------------------------------
# Entry Point
# -----------------------------------------------------------------------------

def main() -> None:
    options = choose_options()

    info = find_device()
    if not info:
        print(
            f"Device {DEVICE_VENDOR_ID:#06x}:{DEVICE_PRODUCT_ID:#06x} not found.",
            file=sys.stderr,
        )
        print("Check: lsusb | grep " + hex(DEVICE_VENDOR_ID)[2:], file=sys.stderr)
        sys.exit(1)

    device_name = (
        f"{info.get('manufacturer_string', '')} "
        f"{info.get('product_string', '')}"
    )
    print(f"\nFound: {device_name}")
    print(f"  Path: {info.get('path', '')}")
    print(f"  UsagePage: 0x{info.get('usage_page', 0):04X}")

    try:
        dev = open_device(DEVICE_VENDOR_ID, DEVICE_PRODUCT_ID)
    except Exception as err:
        print(f"Failed to open device: {err}", file=sys.stderr)
        sys.exit(1)

    try:
        configure_nonblocking(dev)

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
