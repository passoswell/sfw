# Linux DIO (libgpiod)

## Table of contents
- [Overview](#overview)
- [Implemented class](#implemented-class)
- [Dependencies](#dependencies)
- [Setup](#setup)
- [Usage example](#usage-example)
- [Notes](#notes)

## Overview
This folder contains the Linux implementations of the DIO interfaces based on
libgpiod.

The output implementation opens a GPIO chip, requests one line as output, and
supports write and toggle operations.

The input implementation opens a GPIO chip, requests one line as input, and
supports synchronous reads of the current level.

## Implemented class
- `DigitalOutput` (`digital_output.hpp`)
  - Implements `hal_interface::DigitalOutput`.
  - Supports output drive mode (`push-pull`, `open-drain`, `open-source`).
  - Supports line bias selection (`disable`, `pull-down`, `pull-up`).
  - Accepts device name as either `/dev/gpiochipX` or chip name.
- `DigitalInput` (`digital_input.hpp`)
  - Implements `hal_interface::DigitalInput`.
  - Supports line bias selection (`disable`, `pull-down`, `pull-up`).
  - Accepts device name as either `/dev/gpiochipX` or chip name.

## Dependencies
- Linux GPIO character device support.
- libgpiod v1.x (`gpiod.h`, `gpiod_chip_*`, `gpiod_line_*` APIs).
- HAL interface module (`hal_interface::DigitalOutput`,
  `hal_interface::DigitalInput`).

## Setup
1. Ensure your Linux system exposes the GPIO chip device.
2. Verify permissions for the selected gpiochip device.
3. Construct `DigitalOutput` with:
  - Device path/name
  - Line offset
  - Initial state
  - Optional line drive and bias
4. Call `Initialize()` before using `Write()` or `Toggle()`.
5. For inputs, construct `DigitalInput` with the device path/name and line
   offset, then call `Initialize()` before `Read()`.

## Usage example
This example opens GPIO chip 0, requests line 17 as output, drives it high,
toggles it once, then deinitializes the object.

```cpp
#include "hal_linux/dio/digital_output.hpp"

using sfw::hal_linux::DigitalOutput;

void DriveLed() {
  DigitalOutput led(
      "/dev/gpiochip0", 17U, false,
      sfw::hal_interface::DigitalOutput::LineDrive::kPushPull,
      sfw::hal_interface::DigitalOutput::LineBias::kDisable);

  if (led.Initialize() != sfw::hal_interface::ErrorCode::kOk) {
    return;
  }

  (void)led.Write(true, 100U);
  (void)led.Toggle(100U);
  (void)led.Deinitialize();
}
```

This example opens GPIO chip 0, requests line 23 as input, reads its level,
and then deinitializes the object.

```cpp
#include "hal_linux/dio/digital_input.hpp"

using sfw::hal_linux::DigitalInput;

void SampleButton() {
  DigitalInput button("/dev/gpiochip0", 23U);

  if (button.Initialize() != sfw::hal_interface::ErrorCode::kOk) {
    return;
  }

  bool state = false;
  (void)button.Read(state, 100U);
  (void)button.Deinitialize();
}
```

## Notes
- `timeout_ms` in `Write()` is accepted for interface compatibility, but Linux
  libgpiod line writes are immediate and do not use that timeout.
- `Deinitialize()` attempts to restore the initial state before releasing the
  line.
- `timeout_ms` in `Read()` is accepted for interface compatibility, but Linux
  libgpiod line reads are immediate and do not use that timeout.