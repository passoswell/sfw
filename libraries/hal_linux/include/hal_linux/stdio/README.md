# Linux STDIO Serial

## Table of contents
- [Overview](#overview)
- [Implemented class](#implemented-class)
- [Dependencies](#dependencies)
- [Setup](#setup)
- [Usage example](#usage-example)
- [Notes](#notes)

## Overview
This folder contains a Linux implementation of `hal_interface::Serial`
using the process standard input/output streams.

`StdioSerial` is useful for local testing, REPL-like command exchange,
or simulation environments where `/dev/tty*` access is not required.

It supports:
- Timeout-based reads and writes using `poll()`.
- Immediate input handling by switching stdin to raw mode during operation.
- Automatic restoration of terminal settings on `Deinitialize()` and
	destructor.
- Input queue query (`BytesAvailable`) and flush (`ClearReadBuffer`).

## Implemented class
- `StdioSerial` (`stdio_serial.hpp`)
	- Uses `STDIN_FILENO` for reads and `STDOUT_FILENO` for writes.
	- Saves and restores original terminal attributes (`termios`).
	- Saves and restores original file status flags (`fcntl`).
	- Implements both byte (`uint8_t`) and character (`char`) overloads.

## Dependencies
- POSIX/Unix APIs:
	- `termios.h`, `poll.h`, `fcntl.h`, `unistd.h`, `ioctl`
- HAL interface module (`hal_interface::Serial`).
- Interactive terminal (raw mode configuration is only valid when stdin
	is a tty).

## Setup
1. Construct `StdioSerial`.
2. Call `Initialize()` before read/write operations.
3. Exchange data with `Read()` and `Write()`.
4. Call `Deinitialize()` when done.

No additional hardware setup is required.

## Usage example
This example initializes `StdioSerial`, writes a prompt, waits for input,
and then restores terminal settings.

```cpp
#include <array>

#include "hal_linux/stdio/stdio_serial.hpp"

using sfw::hal_linux::StdioSerial;

void EchoOnce() {
	StdioSerial serial;
	if (serial.Initialize() != sfw::hal_interface::ErrorCode::kOk) {
		return;
	}

	constexpr std::array<const char, 9> prompt{"input> \n"};
	(void)serial.Write(std::span<const char>(prompt.data(), prompt.size()),
										 100U);

	std::array<char, 64> rx{};
	uint16_t bytes_read = 0;
	(void)serial.Read(std::span<char>(rx), bytes_read, 5000U);

	(void)serial.Deinitialize();
}
```

## Notes
- `Read()` returns `kTimeout` when no byte arrives before timeout.
- `Write()` returns `kTimeout` when not all bytes are written before timeout.
- If `Initialize()` succeeds, terminal settings are restored either by
	`Deinitialize()` or by object destruction as a fallback.