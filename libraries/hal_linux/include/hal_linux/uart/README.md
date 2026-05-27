# Linux UART Serial (termios)

## Table of contents
- [Overview](#overview)
- [Implemented class](#implemented-class)
- [Dependencies](#dependencies)
- [Setup](#setup)
- [Usage example](#usage-example)
- [Notes](#notes)

## Overview
This folder contains a Linux implementation of `hal_interface::Serial` using
TTY devices and `termios` configuration.

It supports:
- Configurable baud rate, data bits, parity, stop bits, and flow control
- Buffered reads with timeout using `poll()`
- Writes with optional wait for end-of-transmission
- RX buffer query and flush helpers

## Implemented class
- `UartSerial` (`uart_serial.hpp`)
  - Opens tty devices such as `/dev/ttyS0` or `/dev/ttyUSB0`.
  - Configures the line via `termios` (`cfmakeraw`, baud, framing, flow).
  - Implements timeout-based read and write behavior.
  - Exposes `BytesAvailable()` via `ioctl(FIONREAD)`.

## Dependencies
- POSIX/Unix APIs:
  - `termios.h`, `poll.h`, `fcntl.h`, `unistd.h`, `ioctl`
- Linux tty device nodes (for example `/dev/ttyUSB0`).
- HAL interface module (`hal_interface::Serial`).

## Setup
1. Ensure UART/USB-serial device node exists.
2. Ensure user permissions for the tty device.
3. Construct `UartSerial` with desired communication settings.
4. Call `Initialize()` before reading or writing.

## Usage example
This example opens `/dev/ttyUSB0` at `115200 8N1`, sends a command string,
attempts to read a response, then deinitializes the UART object.

```cpp
#include <array>

#include "hal_linux/uart/uart_serial.hpp"

using sfw::hal_linux::UartSerial;

void SendCommandAndReadReply() {
  UartSerial uart("/dev/ttyUSB0", 115200U, true,
                  sfw::hal_interface::Serial::FlowControl::kNone,
                  sfw::hal_interface::Serial::DataBits::k8,
                  sfw::hal_interface::Serial::Parity::kNone,
                  sfw::hal_interface::Serial::StopBits::k1);

  if (uart.Initialize() != sfw::hal_interface::ErrorCode::kOk) {
    return;
  }

  constexpr std::array<const char, 6> cmd{"AT\r\n"};
  (void)uart.Write(std::span<const char>(cmd.data(), 4U), 100U);

  std::array<char, 64> rx{};
  uint16_t bytes_read = 0;
  (void)uart.Read(std::span<char>(rx), bytes_read, 200U);

  (void)uart.Deinitialize();
}
```

## Notes
- Reads return `kTimeout` when no byte arrives within the timeout window.
- When `wait_end_of_transmission` is enabled, write completion is estimated
  from pending kernel TX bytes and baud rate.