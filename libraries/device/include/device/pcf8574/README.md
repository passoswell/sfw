# PCF8574 Device Driver

## Table of contents
- [Overview](#overview)
- [Architecture](#architecture)
- [Dependencies](#dependencies)
- [Setup](#setup)
- [Example 1: Using Pcf8574 directly](#example-1-using-pcf8574-directly)
- [Example 2: Using DigitalInput and DigitalOutput](#example-2-using-digitalinput-and-digitaloutput)
- [Notes](#notes)

## Overview
This module provides support for the PCF8574 and PCF8574A 8-bit I/O expander
over I2C.

It includes:
- `Pcf8574`: low-level access to the full 8-bit port and individual pins.
- `DigitalInput`: `hal_interface::DigitalInput` wrapper for one PCF8574 pin.
- `DigitalOutput`: `hal_interface::DigitalOutput` wrapper for one PCF8574 pin.

The driver handles read/write transactions and pin-level access on top of an
injected `hal_interface::I2cController`.

## Architecture
- `Pcf8574`
  - Initializes the target and verifies it is reachable on the I2C bus.
  - Supports full-port operations (`ReadPort`, `WritePort`).
  - Supports per-pin operations (`ReadPin`, `WritePin`).
- `DigitalInput`
  - Adapts one PCF8574 pin to `hal_interface::DigitalInput`.
  - Initializes the pin in released-high mode for input sampling.
- `DigitalOutput`
  - Adapts one PCF8574 pin to `hal_interface::DigitalOutput`.
  - Maintains output state and supports `Write` and `Toggle`.

## Dependencies
- `hal_interface::I2cController`
  - Used by `Pcf8574` for all bus transactions.
- `hal_interface::DigitalInput`
  - Interface implemented by `DigitalInput`.
- `hal_interface::DigitalOutput`
  - Interface implemented by `DigitalOutput`.

Build requirements:
- C++20 compiler.

## Setup
1. Provide a platform `hal_interface::I2cController` implementation.
2. Select the PCF8574/PCF8574A 7-bit address (for example `0x20` or `0x38`).
3. Construct `Pcf8574` with the I2C controller and address.
4. Either:
  - Use `Pcf8574` directly for pin/port access, or
  - Create `DigitalInput`/`DigitalOutput` wrappers for pin-level interfaces.

## Example 1: Using Pcf8574 directly
This example initializes the expander, reads one input pin, and mirrors that
state to another pin. The input pin is first written high to release it for
sampling, which matches PCF8574 quasi-bidirectional behavior.

```cpp
#include "device/pcf8574/pcf8574.hpp"

// Warning: Using the using namespace directive (especially in C++) is generally
// considered bad practice because it causes namespace pollution and introduces
// severe risks of name collisions.
using sfw::device::pcf8574::I2cAddress;
using sfw::device::pcf8574::Pcf8574;

// These HAL objects are provided by the platform specific hal layer.
// For instance, if on linux platform, one go to the hal_linux folder and
// verify what are the implementation options for the interfaces used by this
// example.
extern hal_interface::I2cController i2c;

void MirrorInputToOutputDirect() {
  constexpr uint8_t kInputPin = 0U;
  constexpr uint8_t kOutputPin = 1U;
  constexpr uint32_t kTimeoutMs = 100U;

  Pcf8574 expander(i2c, static_cast<uint8_t>(I2cAddress::k0x20h));
  if (expander.Initialize() != hal_interface::ErrorCode::kOk) {
    return;
  }

  // Release input pin high so it can be sampled.
  if (expander.WritePin(kInputPin, true, kTimeoutMs)
      != hal_interface::ErrorCode::kOk) {
    return;
  }

  bool input_state = false;
  if (expander.ReadPin(kInputPin, input_state, kTimeoutMs)
      != hal_interface::ErrorCode::kOk) {
    return;
  }

  (void)expander.WritePin(kOutputPin, input_state, kTimeoutMs);
}
```

## Example 2: Using DigitalInput and DigitalOutput
This example performs the same mirror behavior as Example 1, but uses
`DigitalInput` and `DigitalOutput` wrappers so the code works through generic
HAL digital interfaces.

```cpp
#include "device/pcf8574/pcf8574.hpp"
#include "device/pcf8574/pcf8574_digital_input.hpp"
#include "device/pcf8574/pcf8574_digital_output.hpp"

// Warning: Using the using namespace directive (especially in C++) is generally
// considered bad practice because it causes namespace pollution and introduces
// severe risks of name collisions.
using sfw::device::pcf8574::I2cAddress;
using sfw::device::pcf8574::Pcf8574;
using sfw::device::pcf8574::DigitalInput;
using sfw::device::pcf8574::DigitalOutput;

// These HAL objects are provided by the platform specific hal layer.
// For instance, if on linux platform, one go to the hal_linux folder and
// verify what are the implementation options for the interfaces used by this
// example.
extern hal_interface::I2cController i2c;

void MirrorInputToOutputWithAdapters() {
  constexpr uint8_t kInputPin = 0U;
  constexpr uint8_t kOutputPin = 1U;
  constexpr uint32_t kTimeoutMs = 100U;

  Pcf8574 expander(i2c, static_cast<uint8_t>(I2cAddress::k0x20h));
  DigitalInput input(expander, kInputPin);
  DigitalOutput output(expander, kOutputPin, false);

  if (input.Initialize() != hal_interface::ErrorCode::kOk) {
    return;
  }
  if (output.Initialize() != hal_interface::ErrorCode::kOk) {
    return;
  }

  bool input_state = false;
  if (input.Read(input_state, kTimeoutMs) != hal_interface::ErrorCode::kOk) {
    return;
  }

  (void)output.Write(input_state, kTimeoutMs);
}
```

## Notes
- PCF8574 pins are quasi-bidirectional. This means they are equipped with weak
  pull-up resistors. These resistors are probably placed external to the chip.
- Writing `true` to a pin releases it high; external circuitry can then pull it
  low.This is the only way PCF8574 can read external signals.
- If multiple threads/tasks access the same I2C controller, synchronization
  must be handled externally.