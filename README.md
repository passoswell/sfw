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
- `sfw.cpp`: executable entry point and integration examples.

## Requirements
Minimum toolchain:
- CMake 3.23+
- Ninja
- GCC and G++ with C++20 support
- pkg-config

Additional STM32 toolchain requirements:
- `arm-none-eabi-gcc`
- `arm-none-eabi-g++`
- `arm-none-eabi-objcopy`
- `arm-none-eabi-size`

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

STM32 scaffold:

1. Generate a CubeMX CMake project for your target MCU with STM32CubeIDE.
2. Place the generated project under
  `external/cubemx/generic-stm32f401cx`, or override
  `SFW_STM32_CUBEMX_DIR` in a preset or on the command line.
  SFW reads STM32 compile definitions and target CPU/FPU flags directly from
  the CubeMX-generated CMake files, so manual per-part updates in SFW CMake
  are not required.
3. Configure the STM32 preset:

```bash
cmake --preset stm32-debug
```

4. Build:

```bash
cmake --build --preset build-stm32-debug
```

The legacy preset `stm32f401cx-debug` is kept as a compatibility alias for the
current default board path, but the generic `stm32-*` presets are now the
recommended interface.

The CubeMX linker helper supports both common generated layouts:
- startup assembly under `Core/Startup`
- startup assembly at project root (for example `startup_stm32f401xc.s`)

Additional firmware artifacts are emitted beside the ELF during STM32 builds:
- `sfw.hex`
- `sfw.bin`
- `sfw.map`

If the required tools are available on the host, CMake also exposes optional
flash targets for STM32CubeProgrammer and OpenOCD.

The STM32 path expects CubeMX to own startup code, linker script, clock tree,
GPIO configuration, and low-level peripheral initialization.

Run the main executable:

```bash
./build/linux-debug/sfw
```

## Documentation map
- HAL interfaces: [libraries/hal_interface/README.md](libraries/hal_interface/README.md)
- Linux HAL implementations: [libraries/hal_linux/README.md](libraries/hal_linux/README.md)
- STM32 HAL implementations: [libraries/hal_stm32/README.md](libraries/hal_stm32/README.md)
- HD44780 driver: [libraries/device/include/device/hd44780/README.md](libraries/device/include/device/hd44780/README.md)
- JEDEC flash driver: [libraries/device/include/device/jedec_flash/README.md](libraries/device/include/device/jedec_flash/README.md)
- PCF8574 driver: [libraries/device/include/device/pcf8574/README.md](libraries/device/include/device/pcf8574/README.md)

## Current status
Active development.

Core architecture and multiple drivers/backends are already available, with
continuous updates to APIs, documentation, and integration examples.
