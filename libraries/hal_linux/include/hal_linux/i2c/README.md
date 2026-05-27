# Linux I2C Controller

## Table of contents
- [Overview](#overview)
- [Implemented class](#implemented-class)
- [Dependencies](#dependencies)
- [Setup](#setup)
- [Usage example](#usage-example)
- [Notes](#notes)

## Overview
This folder contains a Linux implementation of `hal_interface::I2cController`
using `/dev/i2c-*` devices and Linux I2C ioctls.

It supports:
- Raw read/write transactions
- Combined write-then-read (`I2C_RDWR`)
- Register-addressed read/write helpers
- Device presence detection

## Implemented class
- `I2cController` (`i2c_controller.hpp`)
  - Opens the bus device with `open()`.
  - Selects 7-bit or 10-bit addressing mode.
  - Configures kernel I2C timeout (`I2C_TIMEOUT`).
  - Uses `read()`, `write()`, `ioctl(I2C_RDWR)`, and SMBus probe helpers.

## Dependencies
- Linux kernel I2C userspace API:
  - `linux/i2c-dev.h`
  - `linux/i2c.h`
  - `i2c/smbus.h`
- POSIX APIs (`open`, `read`, `write`, `ioctl`, `close`).
- HAL interface module (`hal_interface::I2cController`).

## Setup
1. Ensure I2C is enabled and target bus appears as `/dev/i2c-X`.
2. Ensure your user has permission to access the bus device.
3. Construct `I2cController` with the selected device path.
4. Call `Initialize()` before any transfer.

## Usage example
This example opens `/dev/i2c-1`, probes a 7-bit target at `0x3C`, writes one
register address (`0x00`), and reads one byte back from that register.

```cpp
#include <array>

#include "hal_linux/i2c/i2c_controller.hpp"

using sfw::hal_linux::I2cController;

void ProbeAndReadOneByte() {
  I2cController i2c("/dev/i2c-1");
  if (i2c.Initialize() != sfw::hal_interface::ErrorCode::kOk) {
    return;
  }

  constexpr uint16_t kAddress = 0x3CU;
  if (i2c.Detect(kAddress, false, 100U)
      != sfw::hal_interface::ErrorCode::kOk) {
    (void)i2c.Deinitialize();
    return;
  }

  std::array<uint8_t, 1> value{};
  (void)i2c.ReadRegister(kAddress, false, 0x00U, 1U,
                         std::span<uint8_t>(value), 100U);

  (void)i2c.Deinitialize();
}
```

## Notes
- Timeout is configured using Linux I2C timeout ticks (`10 ms` granularity).
- `ReadRegister()` and `WriteRegister()` use MSB-first register encoding.
- `WriteRegister()` assembles register+payload into a temporary vector.