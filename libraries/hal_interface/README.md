# HAL Interface Module

## Table of contents
- [Overview](#overview)
- [Design goals](#design-goals)
- [Available interfaces](#available-interfaces)
- [Error model](#error-model)
- [How to implement](#how-to-implement)
- [How to consume](#how-to-consume)
- [Usage example](#usage-example)
- [Notes](#notes)

## Overview
This module defines hardware abstraction layer (HAL) interfaces used by the
project. It provides abstract C++ classes for common peripherals such as GPIO,
I2C, SPI, timers, memory devices, and serial communication.

The interfaces are intentionally implementation-agnostic. Concrete backends
can target Linux, microcontrollers, simulators, or test doubles without
changing higher-level driver code.

## Design goals
- Decouple device drivers from platform-specific code.
- Standardize initialization, deinitialization, and timeout behavior.
- Provide consistent return semantics through `hal_interface::ErrorCode`.
- Keep APIs synchronous/blocking for deterministic control flow.
- Enable easy mocking/faking for unit and integration tests.

## Available interfaces
- `ErrorCode`
  - Common status values used by all HAL APIs.
- `DigitalInput`
  - Read discrete input state.
- `DigitalOutput`
  - Drive discrete output state and toggle output.
- `DigitalInOut`
  - Bidirectional GPIO with runtime direction switching.
- `AnalogInput`
  - Acquire normalized or raw ADC samples.
- `AnalogOutput`
  - Output normalized or raw DAC samples.
- `I2cController`
  - Raw and register-based I2C transactions.
- `SpiController`
  - Raw, transfer, and register-based SPI transactions.
- `Serial`
  - Buffered serial read/write APIs.
- `Timer`
  - Hardware timer control, counters, and callbacks.
- `SoftwareTimer`
  - Blocking delay and elapsed/expiration time utilities.
- `PWM`
  - Duty-cycle output control (normalized and raw).
- `Memory`
  - Generic random-access read/write/erase API for memory devices.

## Error model
All HAL methods return `hal_interface::ErrorCode`:
- `kOk`: operation succeeded.
- `kInvalidArgument`: invalid argument values.
- `kOutOfRange`: argument outside valid limits.
- `kTimeout`: operation did not complete before timeout.
- `kError`: generic or device-specific failure.
- `kNotSupported`: feature not supported by implementation.

## How to implement
1. Create a concrete class deriving from one HAL interface.
2. Implement all pure virtual methods with platform-specific logic.
3. Respect method contract semantics from the interface documentation.
4. Enforce timeout behavior for blocking operations.
5. Return standardized `ErrorCode` values consistently.
6. Keep the class small and focused on one peripheral abstraction.

## How to consume
1. Depend on interfaces in drivers and application logic, not concrete classes.
2. Inject concrete implementations through constructors.
3. Call `Initialize()` once before using an interface instance.
4. Check return values for every operation.
5. Call `Deinitialize()` when the peripheral is no longer needed.

## Usage example
This example shows a small routine that initializes a digital input and output,
reads the input state, mirrors it to the output, then deinitializes both
interfaces while checking return codes.

```cpp
#include "hal_interface/digital_input.hpp"
#include "hal_interface/digital_output.hpp"

hal_interface::ErrorCode MirrorInputToOutput(
    sfw::hal_interface::DigitalInput& input,
    sfw::hal_interface::DigitalOutput& output,
    uint32_t timeout_ms) {
  using sfw::hal_interface::ErrorCode;

  ErrorCode result = input.Initialize();
  if (result != ErrorCode::kOk) {
    return result;
  }

  result = output.Initialize();
  if (result != ErrorCode::kOk) {
    (void)input.Deinitialize();
    return result;
  }

  bool state = false;
  result = input.Read(state, timeout_ms);
  if (result == ErrorCode::kOk) {
    result = output.Write(state, timeout_ms);
  }

  const ErrorCode output_deinit_result = output.Deinitialize();
  const ErrorCode input_deinit_result = input.Deinitialize();

  if (result != ErrorCode::kOk) {
    return result;
  }
  if (output_deinit_result != ErrorCode::kOk) {
    return output_deinit_result;
  }
  return input_deinit_result;
}
```

## Notes
- This library only defines interfaces; it does not provide hardware-specific
  implementations.
- Platform implementations should live in dedicated backend modules and link
  against this interface target.
- Since APIs are blocking, time-sensitive applications should choose timeout
  values carefully and consider task scheduling impact.