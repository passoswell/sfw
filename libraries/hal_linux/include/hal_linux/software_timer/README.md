# Chrono Software Timer (Linux)

## Table of contents
- [Overview](#overview)
- [Implemented class](#implemented-class)
- [Dependencies](#dependencies)
- [Setup](#setup)
- [Usage example](#usage-example)
- [Notes](#notes)

## Overview
This folder provides a Linux implementation of `hal_interface::SoftwareTimer`
using `std::chrono::steady_clock` as the time base.

The implementation is thread-safe (internal mutex) and supports:
- One-shot and auto-reload countdown modes
- Start/stop/resume
- Blocking delay
- Elapsed and remaining time queries
- Uptime since `Initialize()`

## Implemented class
- `ChronoSoftwareTimer` (`chrono_software_timer.hpp`)
  - Time source: `std::chrono::steady_clock`
  - Delay primitive: `std::this_thread::sleep_for`
  - Lazy countdown evaluation (no worker thread)

## Dependencies
- C++ standard library (`chrono`, `thread`, `mutex`).
- HAL interface module (`hal_interface::SoftwareTimer`).

## Setup
1. Construct `ChronoSoftwareTimer`.
2. Call `Initialize()`.
3. Optionally call `EnableAutoReload(true)` for periodic mode.
4. Call `Start()` with the desired duration and unit.
5. Query timer state with `HasExpired()` / `GetTimeUntilExpiration()`.

## Usage example
This example starts a one-shot timer for 250 milliseconds, waits until it
expires by polling `HasExpired()`, and then deinitializes the timer.

```cpp
#include "hal_linux/software_timer/chrono_software_timer.hpp"

using sfw::hal_linux::ChronoSoftwareTimer;
using sfw::hal_interface::TimeUnit;

void WaitForOneShot() {
  ChronoSoftwareTimer timer;
  if (timer.Initialize() != sfw::hal_interface::ErrorCode::kOk) {
    return;
  }

  (void)timer.EnableAutoReload(false);
  if (timer.Start(250U, TimeUnit::kMilliseconds)
      != sfw::hal_interface::ErrorCode::kOk) {
    (void)timer.Deinitialize();
    return;
  }

  while (!timer.HasExpired()) {
    (void)timer.Delay(5U, TimeUnit::kMilliseconds);
  }

  (void)timer.Deinitialize();
}
```

## Notes
- If auto-reload is enabled, `HasExpired()` reports true once per elapsed
  period since the previous call.
- In one-shot mode, elapsed-time queries are clamped to the configured
  countdown duration after expiration.