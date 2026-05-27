# HD44780 Device Driver

## Table of contents
- [Overview](#overview)
- [Supported configurations](#supported-configurations)
- [Dependencies](#dependencies)
- [Setup and wiring](#setup-and-wiring)
- [Usage example](#usage-example)
- [Available API](#available-api)
- [Notes](#notes)

## Overview
This module provides a high-level, write-only driver for
HD44780-compatible character LCD controllers.

The driver is split into two layers:
- Bus layer: low-level electrical signaling and timing.
  - 4-bit bus: `Hd44780Bus4Bits`
  - 8-bit bus: `Hd44780Bus8Bits`
- Device layer: high-level display operations.
  - `Hd44780`

The implementation is blocking and uses a software timer to satisfy the
controller timing requirements. Busy-flag reads are intentionally not used.

## Supported configurations
- Bus width:
  - 4-bit mode (D4..D7)
  - 8-bit mode (D0..D7)
- Geometries:
  - 8x1
  - 16x1
  - 16x2
  - 20x2
  - 20x4
  - 40x2
- Optional WR/RW pin:
  - If a pin instance is provided, it is driven low for write-only mode.
  - If omitted, WR/RW is assumed tied to ground in hardware.

## Dependencies
- `hal_interface::DigitalOutput`
  - Used for RS, EN, data pins, and optional WR pin control.
- `hal_interface::SoftwareTimer`
  - Used for command/data timing delays.
- `hal_interface::ErrorCode`
  - Used as the return type for all operations.

Build requirements:
- C++20 compiler (for `std::span` and `std::string_view`).

## Setup and wiring
### 4-bit bus wiring
- Connect LCD RS to the provided RS digital output.
- Connect LCD EN to the provided EN digital output.
- Connect LCD D4, D5, D6, D7 to the corresponding digital outputs.
- Optional: connect LCD WR/RW to a digital output (or tie to GND).

### 8-bit bus wiring
- Connect LCD RS to the provided RS digital output.
- Connect LCD EN to the provided EN digital output.
- Connect LCD D0..D7 to the corresponding digital outputs.
- Optional: connect LCD WR/RW to a digital output (or tie to GND).

### Software initialization sequence
1. Construct a bus object (`Hd44780Bus4Bits` or `Hd44780Bus8Bits`).
2. Construct the high-level `Hd44780` object with the bus, timer, and geometry.
3. Call `Initialize()` once before any display operations.
4. Use `WriteText`, `SetCursor`, and display-control APIs.
5. Call `Deinitialize()` during shutdown.

## Usage example
This example initializes a 16x2 display in 4-bit mode, clears the screen,
writes one line on row 0 and another line on row 1, then enables the cursor
while keeping blink disabled.

```cpp
#include "device/hd44780/hd44780.hpp"
#include "device/hd44780/hd44780_bus_4_bits.hpp"

// Warning: Using the using namespace directive (especially in C++) is generally
// considered bad practice because it causes namespace pollution and introduces
// severe risks of name collisions.
using sfw::device::hd44780::Geometry;
using sfw::device::hd44780::Hd44780;
using sfw::device::hd44780::Hd44780Bus4Bits;

// These HAL objects are provided by the platform specific hal layer.
// For instance, if on linux platform, one can instantiate one
// hal_linux::dio::DigitalOutput for every LCD pin and a
// hal_linux::software_timer::ChronoSoftwareTimer for the timer
extern hal_interface::DigitalOutput rs_pin;
extern hal_interface::DigitalOutput en_pin;
extern hal_interface::DigitalOutput d4_pin;
extern hal_interface::DigitalOutput d5_pin;
extern hal_interface::DigitalOutput d6_pin;
extern hal_interface::DigitalOutput d7_pin;
extern hal_interface::SoftwareTimer system_timer;

void RunLcdDemo() {
  Hd44780Bus4Bits bus(rs_pin, en_pin, d4_pin, d5_pin, d6_pin, d7_pin,
                      system_timer);
  Hd44780 lcd(bus, system_timer, Geometry::k16x2);

  if (lcd.Initialize() != hal_interface::ErrorCode::kOk) {
    return;
  }

  (void)lcd.Clear();
  (void)lcd.SetCursor(0U, 0U);
  (void)lcd.WriteText("Hello");
  (void)lcd.SetCursor(1U, 0U);
  (void)lcd.WriteText("HD44780");

  (void)lcd.SetCursorEnabled(true);
  (void)lcd.SetBlinkEnabled(false);
}
```

## Available API
High-level operations from `Hd44780`:
- Lifecycle:
  - `Initialize()`
  - `Deinitialize()`
  - `IsInitialized()`
- Cursor and text:
  - `Clear()`
  - `Home()`
  - `SetCursor(row, column)`
  - `WriteChar(character)`
  - `WriteText(const char*)`
  - `WriteText(std::string_view)`
- Display modes:
  - `SetDisplayEnabled(enabled)`
  - `SetCursorEnabled(enabled)`
  - `SetBlinkEnabled(enabled)`
  - `SetEntryMode(increment, shift_display)`
  - `ShiftDisplay(right)`
- Custom characters:
  - `CreateCustomChar(slot, bitmap)`
  - `WriteCustomChar(slot)`

## Notes
- This implementation is write-only by design.
- Delays are handled internally with the injected timer.
- For deterministic behavior, always check return codes and stop on first
  failure.