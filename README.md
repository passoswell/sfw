# SFW - System Firmware

Layered C++ firmware architecture with reusable HAL interfaces, Linux HAL
implementations, and device drivers.

## Table of contents
- [Overview](#overview)
- [Repository layout](#repository-layout)
- [Requirements](#requirements)
- [Build and run](#build-and-run)
- [Documentation map](#documentation-map)
- [Current status](#current-status)

## Overview
SFW organizes firmware into three main layers:
- HAL interfaces (`hal_interface`): platform-agnostic contracts.
- HAL implementations (`hal_linux`): Linux backends for selected interfaces.
- Device drivers (`device`): reusable drivers that depend on HAL interfaces.

The project is built with CMake and uses presets for common Linux
configurations.

## Repository layout
- `libraries/hal_interface`: abstract interfaces for peripherals and services.
- `libraries/hal_linux`: Linux implementations (GPIO, I2C, SPI, UART, timer).
- `libraries/device`: device drivers (for example HD44780, PCF8574, JEDEC
  flash, W25Q80).
- `cmake_presets`: CMake preset definitions.
- `sfw.cpp`: executable entry point and integration examples.

## Requirements
Minimum toolchain:
- CMake 3.23+
- Ninja
- GCC and G++ with C++20 support
- pkg-config

Linux packages typically required:
- build-essential
- libgpiod-dev (`>=1.0` and `<2.0`)
- libi2c-dev

Useful optional tools:
- gpiod
- i2c-tools
- gdb

## Build and run
From the repository root:

1. List available configure presets:

```bash
cmake --list-presets
```

2. Configure one preset (recommended for development):

```bash
cmake --preset linux-debug
```

3. Build:

```bash
cmake --build --preset build-linux-debug
```

You can also build directly from the generated folder:

```bash
cmake --build build/linux-debug
```

Run the main executable:

```bash
./build/linux-debug/sfw
```

## Documentation map
- HAL interfaces: [libraries/hal_interface/README.md](libraries/hal_interface/README.md)
- Linux HAL implementations: [libraries/hal_linux/README.md](libraries/hal_linux/README.md)
- HD44780 driver: [libraries/device/include/device/hd44780/README.md](libraries/device/include/device/hd44780/README.md)
- JEDEC flash driver: [libraries/device/include/device/jedec_flash/README.md](libraries/device/include/device/jedec_flash/README.md)
- PCF8574 driver: [libraries/device/include/device/pcf8574/README.md](libraries/device/include/device/pcf8574/README.md)

## Current status
Active development.

Core architecture and multiple drivers/backends are already available, with
continuous updates to APIs, documentation, and integration examples.
