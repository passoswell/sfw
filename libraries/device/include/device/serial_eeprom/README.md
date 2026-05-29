# Serial EEPROM

Generic serial EEPROM support split into transport abstraction, concrete I2C
transport, and a `hal_interface::Memory` implementation.

This module targets memories with compatible command/address organization and
different capacities and buses.

For simple EEPROMs in this family, no explicit opcodes are used; transfers are
framed as address bytes (MSB first) followed by data.

Examples:
- 24C32 (I2C)
- 24AA00 / 24LC00 / 24C00 (I2C, smaller address field)
- 25AA010A / 25LC010A (compatible command layout via another transport)

## Components

1. `SerialEepromTransportInterface`
- Abstract bus layer with address-based read/write and ready polling.

2. `SerialEepromI2cTransport`
- Concrete transport using `hal_interface::I2cController`.
- Handles transfer and ready polling using target ACK detection.

3. `SerialEeprom`
- Main class implementing `hal_interface::Memory`.
- Injects transport and `hal_interface::MemoryMetadata`.
- Handles command/address encoding, page-aware chunking, and timeouts.

## Include Paths

- `device/serial_eeprom/interface/serial_eeprom_transport_interface.hpp`
- `device/serial_eeprom/serial_eeprom_i2c_transport.hpp`
- `device/serial_eeprom/serial_eeprom.hpp`

## Configuration Model

`SerialEeprom` gets:
- `SerialEepromTransportInterface&` transport dependency
- `hal_interface::MemoryMetadata` geometry/timing
- `MemoryMetadata::address_size_bytes` (MSB-first address preamble size)

`SerialEepromI2cTransport` gets:
- `hal_interface::I2cController&`
- `hal_interface::SoftwareTimer&`
- `hal_interface::MemoryMetadata` (address width source)
- Individual constructor parameters: target address, address mode and poll
timings

## Usage Example (I2C, 24C32-like)

```cpp
#include <array>

#include "device/serial_eeprom/serial_eeprom.hpp"
#include "device/serial_eeprom/serial_eeprom_i2c_transport.hpp"
#include "hal_interface/memory.hpp"

using sfw::device::serial_eeprom::SerialEeprom;
using sfw::device::serial_eeprom::SerialEepromI2cTransport;
using sfw::hal_interface::MemoryMetadata;

MemoryMetadata metadata{};
metadata.base_address = 0U;
metadata.size_bytes = 4096U;
metadata.address_size_bytes = 2U;
metadata.write_unit = 1U;
metadata.read_block_size = 4096U;  // We can read as many bytes as we want
metadata.write_block_size = 32U;  // The memory can program up to 32 bytes at a time
metadata.erase_block_size = 32U;
metadata.write_alignment = 1U;
metadata.erase_block_alignment = 32U;
metadata.requires_erase_before_program = false;
metadata.maximum_write_time_us = 20000U;
metadata.maximum_sector_erase_time_ms = 20U;
metadata.maximum_memory_erase_time_ms = 4096U/32U;
metadata.erase_cycle_limit = 0U;

SerialEepromI2cTransport transport(
	i2c_controller,
	0x50U,
	false,
	metadata,
	software_timer,
	1U,
	1U);
SerialEeprom eeprom(transport, metadata);

const auto init_result = eeprom.Initialize();
if (init_result != sfw::hal_interface::ErrorCode::kOk) {
	return;
}

std::array<uint8_t, 8> write_data{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
std::array<uint8_t, 8> read_data{};

(void)eeprom.Write(0U, write_data, 50U);
(void)eeprom.Read(0U, read_data, 50U);

(void)eeprom.Deinitialize();
```