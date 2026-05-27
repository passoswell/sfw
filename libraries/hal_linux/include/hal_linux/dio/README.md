# Linux DIO Output (libgpiod)

## Table of contents
- [Overview](#overview)
- [Implemented class](#implemented-class)
- [Dependencies](#dependencies)
- [Setup](#setup)
- [Usage example](#usage-example)
- [Notes](#notes)

## Overview
This folder contains the Linux implementation of
`hal_interface::DigitalOutput` based on libgpiod.

The implementation opens a GPIO chip, requests one line as output, and supports
write and toggle operations.

## Implemented class
- `DigitalOutput` (`digital_output.hpp`)
  - Implements `hal_interface::DigitalOutput`.
  - Supports output drive mode (`push-pull`, `open-drain`, `open-source`).
  - Supports line bias selection (`disable`, `pull-down`, `pull-up`).
  - Accepts device name as either `/dev/gpiochipX` or chip name.

## Dependencies
- Linux GPIO character device support.
- libgpiod v1.x (`gpiod.h`, `gpiod_chip_*`, `gpiod_line_*` APIs).
- HAL interface module (`hal_interface::DigitalOutput`).

## Setup
1. Ensure your Linux system exposes the GPIO chip device.
2. Verify permissions for the selected gpiochip device.
3. Construct `DigitalOutput` with:
  - Device path/name
  - Line offset
  - Initial state
  - Optional line drive and bias
4. Call `Initialize()` before using `Write()` or `Toggle()`.

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

## Notes
- `timeout_ms` in `Write()` is accepted for interface compatibility, but Linux
  libgpiod line writes are immediate and do not use that timeout.
- `Deinitialize()` attempts to restore the initial state before releasing the
  line.