# SFW - System Firmware

Layered C++ firmware architecture with reusable HAL interfaces, Linux HAL
implementations, and device drivers.

## Table of contents
- [Overview](#overview)
- [Repository layout](#repository-layout)
- [Requirements](#requirements)
- [Platform guides](#platform-guides)
- [Current status](#current-status)

## Overview
SFW organizes firmware into three main layers:
- HAL interfaces (`hal_interface`): platform-agnostic contracts.
- HAL implementations (`hal_linux`, `hal_stm32`): platform backends for
  selected interfaces.
- Device drivers (`device`): reusable drivers that depend on HAL interfaces.

The project is built with CMake and uses presets for common Linux
configurations.

## Repository layout
- `libraries/hal_interface`: abstract interfaces for peripherals and services.
- `libraries/hal_linux`: Linux implementations (GPIO, I2C, SPI, UART, timer).
- `libraries/hal_stm32`: STM32 implementations linked against STM32CubeMX
  generated code.
- `libraries/device`: device drivers (for example HD44780, PCF8574, JEDEC
  flash, W25Q80).
- `cmake_presets`: CMake preset definitions.

## Requirements
Common build tools:
- CMake 3.23+
- Ninja
- GCC and G++ with C++20 support
- pkg-config

Linting tools required:
- cppcheck
- clang-tidy
- clang-format

Linux host packages typically required:
- build-essential
- libgpiod-dev (`>=1.0` and `<2.0`)
- libi2c-dev
- libpthread-stubs0-dev (usually covered by system toolchain)

STM32 host requirements:
- `SM32 CubeMX`
- `arm-none-eabi-gcc` (Probably comes with CubeMX)
- `arm-none-eabi-g++` (Probably comes with CubeMX)
- `arm-none-eabi-objcopy` (Probably comes with CubeMX)
- `arm-none-eabi-size` (Probably comes with CubeMX)
- `STM32_Programmer_CLI` (STM32CubeProgrammer)
- `ST-LINK_gdbserver` (STM32CubeCLT or equivalent ST package)

Optional STM32 tools:
- `openocd` (for OpenOCD-based flash/debug workflows)

Useful optional tools:
- gpiod
- i2c-tools
- gdb

## Platform guides
Top-level build flow is intentionally minimal. Platform-specific setup and run
instructions are documented in the module READMEs:

- Linux setup, dependencies, and build/run:
  [libraries/hal_linux/README.md](libraries/hal_linux/README.md)
- STM32 setup, CubeMX integration, build/flash/debug:
  [libraries/hal_stm32/README.md](libraries/hal_stm32/README.md)

Common preset discovery command (from repository root):

```bash
cmake --list-presets
```

Default preset families:
- Linux: `linux-*`
- STM32: `stm32-*`

Current default STM32 preset path points to:
- `external/cubemx/generic`

If you use a different CubeMX project location, override
`SFW_STM32_CUBEMX_DIR` in a preset or on the command line.

## Current status
Active development.

Core architecture and multiple drivers/backends are already available, with
continuous updates to APIs, documentation, and integration examples.
