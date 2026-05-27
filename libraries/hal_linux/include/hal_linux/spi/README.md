# Linux SPI Controller (spidev)

## Table of contents
- [Overview](#overview)
- [Implemented class](#implemented-class)
- [Dependencies](#dependencies)
- [Setup](#setup)
- [Usage example](#usage-example)
- [Notes](#notes)

## Overview
This folder contains a Linux implementation of `hal_interface::SpiController`
using the `spidev` userspace interface.

Supported operations include:
- Read and write transfers
- Full-duplex transfer
- Write-then-read transactions
- Register-addressed read/write helpers

## Implemented class
- `SpiController` (`spi_controller.hpp`)
  - Opens a spidev device (for example `/dev/spidev0.0`).
  - Applies SPI mode, bits-per-word, and clock with `ioctl`.
  - Executes transfers through `SPI_IOC_MESSAGE`.
  - Uses an injected `hal_interface::DigitalOutput` as chip-select control.

## Dependencies
- Linux SPI userspace API:
  - `linux/spi/spidev.h`
- POSIX APIs (`open`, `ioctl`, `close`).
- HAL interface module (`hal_interface::SpiController`).
- A concrete `hal_interface::DigitalOutput` implementation for CS handling.

## Setup
1. Ensure SPI is enabled and exposes `/dev/spidevX.Y`.
2. Ensure permissions allow opening the spidev node.
3. Construct `SpiController` with:
  - Device path
  - SPI mode
  - Clock frequency
  - Bits per word (optional, default 8)
4. Call `Initialize()` before transfers.

## Usage example
This example initializes SPI bus `/dev/spidev0.0`, asserts a dedicated CS pin
through a HAL digital output implementation, sends command `0x9F`, and reads
three response bytes from the target.

```cpp
#include <array>

#include "hal_linux/spi/spi_controller.hpp"

using sfw::hal_linux::SpiController;

void ReadJedecId(hal_interface::DigitalOutput& cs) {
  SpiController spi("/dev/spidev0.0", 0U, 1000000U, 8U);
  if (spi.Initialize() != sfw::hal_interface::ErrorCode::kOk) {
    return;
  }

  std::array<uint8_t, 1> cmd{0x9FU};
  std::array<uint8_t, 3> id{};

  (void)spi.WriteThenRead(cs, false, std::span<const uint8_t>(cmd),
                          std::span<uint8_t>(id), 100U);

  (void)spi.Deinitialize();
}
```

## Notes
- The Linux SPI API does not provide per-transfer timeout support, so
  `timeout_ms` is currently accepted for interface compatibility only.
- Register helper methods serialize register addresses MSB-first.