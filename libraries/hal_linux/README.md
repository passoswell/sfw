# HAL Linux Module

> Linux implementations of HAL interfaces for GPIO, I2C, SPI, UART, and
> software timers.

## Table of contents
- [Overview](#overview)
- [Implemented modules](#implemented-modules)
- [Dependencies](#dependencies)
- [Peripheral access on Linux](#peripheral-access-on-linux)
- [Build](#build)
- [Build and Debug in VS Code](#build-and-debug-in-vs-code)
- [Notes](#notes)

## Overview
`hal_linux` provides concrete Linux backends for interfaces defined in
`hal_interface`.

These implementations are intended for Linux SBCs and desktop Linux systems
where peripheral access is available through device nodes such as `/dev/i2c-*`,
`/dev/spidev*`, `/dev/tty*`, and `/dev/gpiochip*`.

## Implemented modules
- DIO output via libgpiod:
  - `include/hal_linux/dio/digital_output.hpp`
  - [Module README](libraries/hal_linux/include/hal_linux/dio/README.md)
- I2C controller via Linux I2C userspace API:
  - `include/hal_linux/i2c/i2c_controller.hpp`
  - [Module README](libraries/hal_linux/include/hal_linux/i2c/README.md)
- SPI controller via spidev:
  - `include/hal_linux/spi/spi_controller.hpp`
  - [Module README](libraries/hal_linux/include/hal_linux/spi/README.md)
- Software timer via `std::chrono`:
  - `include/hal_linux/software_timer/chrono_software_timer.hpp`
  - [Module README](libraries/hal_linux/include/hal_linux/software_timer/README.md)
- STDIO serial backend via stdin/stdout:
  - `include/hal_linux/stdio/stdio_serial.hpp`
- UART serial via termios:
  - `include/hal_linux/uart/uart_serial.hpp`
  - [Module README](libraries/hal_linux/include/hal_linux/uart/README.md)

## Dependencies
Required build tools and libraries (Debian/Ubuntu):
- `cmake` (3.23+)
- `ninja-build`
- `build-essential`
- `pkg-config`
- `libgpiod-dev` (version `>=1.0` and `<2.0`)
- `libi2c-dev`
- `libpthread-stubs0-dev` (usually covered by system toolchain)

Linting tools required:
- `cppcheck`
- `clang-tidy`
- `clang-format`

Useful runtime/debug tools:
- `gpiod`
- `i2c-tools`
- `gdb`

Example installation command:

```bash
sudo apt update -y && sudo apt upgrade -y
sudo apt install -y cmake ninja-build build-essential pkg-config \
  libgpiod-dev libi2c-dev gpiod i2c-tools gdb
```

## Peripheral access on Linux
Common peripheral device prefixes:
- `gpiochip` for GPIO controller banks
- `i2c-` for I2C buses
- `spidev` for SPI buses
- `tty` for UART and USB-serial ports

If device nodes are missing, enable peripherals in your board/distribution
configuration (often through boot config or device-tree overlays).

If permission is denied, add your user to the proper group and re-login:

```bash
sudo usermod -a -G gpio,i2c,spi,dialout <user_name>
```

If a group does not exist on your system, create it first:

```bash
sudo groupadd gpio
sudo groupadd spi
sudo groupadd i2c
```

## Build
From repository root (`sfw`):

1. List available presets:

```bash
cmake --list-presets
```

2. Configure one Linux preset (recommended):

```bash
cmake --preset linux-debug
```

Alternative presets are available, such as `linux-release`,
`linux-release-o2`, and `linux-release-o3`.

3. Build:

```bash
cmake --build --preset build-linux-debug
```

You can also build directly from the configure output directory:

```bash
cmake --build build/linux-debug
```

## Build and Debug in VS Code
Required VS Code extensions:
- CMake Tools (`ms-vscode.cmake-tools`)
- C/C++ (`ms-vscode.cpptools`)

Recommended extension:
- C/C++ Extension Pack (`ms-vscode.cpptools-extension-pack`)

Step-by-step (Linux target):
1. Open the repository root in VS Code.
2. Ensure host dependencies from [Dependencies](#dependencies) are installed.
3. Open Command Palette and run `CMake: Select Configure Preset`, then choose
   `linux-debug`.
4. Run `CMake: Configure`.
5. Run `CMake: Build` (or use the Build button from CMake Tools).
6. Set launch target to `linux_tests` from the CMake status bar item.
7. Use Run (without debugger) to start the binary.
8. Use Debug to start a GDB debug session.

Troubleshooting:
- If configure fails, verify compilers, CMake, and Ninja are installed.
- If debug does not start, verify `gdb` is installed and the selected preset is
  a Linux preset.
- If runtime access to `/dev/gpiochip*`, `/dev/i2c-*`, or `/dev/spidev*` fails,
  revisit group membership and permissions in
  [Peripheral access on Linux](#peripheral-access-on-linux).

## Notes
- The `hal_linux` target is built in `libraries/hal_linux/CMakeLists.txt` and
  links against `hal_interface`.
- SPI transfer timeout arguments are accepted for interface compatibility, but
  Linux spidev does not provide native per-transfer timeout control.
