# JEDEC-Compatible SPI Flash Memory Driver

## Table of contents
- [Overview](#overview)
- [Architecture](#architecture)
- [Dependencies](#dependencies)
- [Supported functionality](#supported-functionality)
- [Setup](#setup)
- [Usage example](#usage-example)
- [Notes and limitations](#notes-and-limitations)

## Overview
This module provides a JEDEC-compatible SPI NOR flash driver in two layers:
- `JedecSpiFlash`: low-level opcode transport over SPI.
- `JedecSpiFlashMemory`: `hal_interface::Memory` adapter for read/write/erase
  operations with metadata-aware chunking and validation.

The driver supports JEDEC ID read and SFDP/BFPT discovery to populate memory
geometry and timing metadata when not provided explicitly.

## Architecture
### Low-level transport: `JedecSpiFlash`
Responsibilities:
- Initialize/deinitialize SPI, CS pin, and software timer dependencies.
- Issue JEDEC-compatible commands:
  - Read JEDEC ID (`0x9F`)
  - SFDP read (`0x5A`)
  - Read data
  - Page program
  - Sector erase
  - Chip erase
- Poll device busy flag through status register reads.

### Memory adapter: `JedecSpiFlashMemory`
Responsibilities:
- Expose generic `hal_interface::Memory` API.
- Validate ranges and alignment against `MemoryMetadata`.
- Split transfers into safe chunks (read block size, page size).
- Translate logical addresses to device addresses.
- Optionally auto-populate metadata from JEDEC/SFDP information.

## Dependencies
- `hal_interface::SpiController`
  - SPI transactions for command/data transfer.
- `hal_interface::DigitalOutput`
  - Chip-select pin control.
- `hal_interface::SoftwareTimer`
  - Timeout handling and busy polling delay.
- `hal_interface::Memory` and `hal_interface::MemoryMetadata`
  - Generic memory API integration and geometry constraints.

Build requirements:
- C++20 compiler.

## Supported functionality
- JEDEC ID read and SFDP parsing.
- BFPT-based extraction of:
  - Memory size
  - Addressing capability
  - Page size
  - Erase block size
  - Program/erase timing limits
- Blocking operations with timeout arguments:
  - Read
  - Write (page program)
  - Erase sector/block
  - Erase whole chip

## Setup
1. Create and configure platform HAL objects:
  - SPI controller
  - CS digital output
  - Software timer
2. Construct `JedecSpiFlash` with HAL dependencies.
3. Construct `JedecSpiFlashMemory`:
  - With explicit `MemoryMetadata`, or
  - Without metadata to use SFDP auto-discovery during `Initialize()`.
4. Call `Initialize()` before any memory operation.
5. Use `Read()`, `Write()`, `EraseBlock()`, and `EraseAllMemory()`.
6. Call `Deinitialize()` when finished.

## Usage example
This example initializes the flash memory adapter with JEDEC/SFDP metadata
discovery, derives operation-specific timeouts from metadata, erases one block,
programs 256 bytes at address 0, and reads the same 256 bytes back for
verification.

```cpp
#include <algorithm>

#include "device/jedec_flash/jedec_spi_flash.hpp"
#include "device/jedec_flash/jedec_spi_flash_memory.hpp"

// Warning: Using the using namespace directive (especially in C++) is generally
// considered bad practice because it causes namespace pollution and introduces
// severe risks of name collisions.
using sfw::device::jedec_flash::JedecSpiFlash;
using sfw::device::jedec_flash::JedecSpiFlashMemory;

// These HAL objects are provided by the platform specific hal layer.
// For instance, if on linux platform, one go to the hal_linux folder and
// verify what are the implementation options for the interfaces used by this
// example.
extern hal_interface::SpiController spi;
extern hal_interface::DigitalOutput cs_pin;
extern hal_interface::SoftwareTimer timer;

void FlashDemo() {
  constexpr bool kCsActiveLow = false;

  JedecSpiFlash flash(spi, cs_pin, kCsActiveLow, timer);
  JedecSpiFlashMemory memory(flash);  // Uses JEDEC/SFDP metadata discovery.

  if (memory.Initialize() != hal_interface::ErrorCode::kOk) {
    return;
  }

  const hal_interface::MemoryMetadata metadata = memory.GetMetadata();
  const auto UsToMsCeil = [](uint32_t microseconds) {
    return (microseconds + 999U) / 1000U;
  };

  const uint32_t WriteTimeoutMs =
      std::max<uint32_t>(1U, UsToMsCeil(metadata.maximum_write_time_us));
  const uint32_t EraseBlockTimeoutMs =
      std::max<uint32_t>(1U, metadata.maximum_sector_erase_time_ms);
  const uint32_t EraseAllTimeoutMs =
      std::max<uint32_t>(1U, metadata.maximum_memory_erase_time_ms);

  // MemoryMetadata does not expose a dedicated read timing field.
  // Use write timeout as a conservative per-transaction read timeout.
  const uint32_t ReadTimeoutMs = WriteTimeoutMs;

  std::array<uint8_t, 256> write_data{};
  std::array<uint8_t, 256> read_data{};
  for (size_t i = 0; i < write_data.size(); ++i) {
    write_data[i] = static_cast<uint8_t>(i & 0xFFU);
  }

  constexpr uint64_t kAddress = 0U;

  (void)memory.EraseBlock(kAddress, EraseBlockTimeoutMs);
  (void)memory.Write(
      kAddress, std::span<const uint8_t>(write_data), WriteTimeoutMs);
  (void)memory.Read(kAddress, std::span<uint8_t>(read_data), ReadTimeoutMs);

  // If needed:
  // (void)memory.EraseAllMemory(EraseAllTimeoutMs);
}
```

## Notes and limitations
- Operations are blocking.
- The memory adapter currently uses 3-byte addresses for underlying flash
  transactions.
- Correctness of write/erase alignment depends on accurate metadata.
- When auto-discovery is used, metadata depends on JEDEC/SFDP data quality from
  the target device.