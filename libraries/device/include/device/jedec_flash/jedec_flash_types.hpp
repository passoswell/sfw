// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef DEVICE_JEDEC_FLASH_JEDEC_FLASH_TYPES_HPP
#define DEVICE_JEDEC_FLASH_JEDEC_FLASH_TYPES_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace sfw::device::jedec_flash {

/**
 * @brief JEDEC-compatible discovery information.
 *
 * Data gathered from Read JEDEC ID (0x9F) and SFDP Read (0x5A).
 * Timing-related fields are exposed as raw BFPT DWORDs to avoid
 * misinterpreting vendor-specific encodings.
 */
struct Info {
  static constexpr size_t kMaxBfptDwords{16U};

  /**
   * @brief Addressing mode support from BFPT DWORD1 bits [18:17].
   */
  enum class AddressByteSupport : uint8_t {
    k3BytesOnly = 0,
    k3Or4Bytes = 1,
    k4BytesOnly = 2,
    kReserved = 3,
  };

  std::array<uint8_t, 3> jedec_id{};  ///< Manufacturer/type/capacity bytes.
  bool sfdp_available{false};
  uint8_t sfdp_major_revision{0U};
  uint8_t sfdp_minor_revision{0U};
  uint8_t sfdp_parameter_header_count{0U};
  uint8_t configured_address_size_bytes{0U};
  AddressByteSupport address_byte_support{AddressByteSupport::kReserved};
  uint64_t density_bits{0U};
  uint64_t memory_size_bytes{0U};
  uint32_t bfpt_pointer{0U};
  uint8_t bfpt_dword_count_available{0U};
  uint8_t bfpt_dword_count_read{0U};
  std::array<uint32_t, kMaxBfptDwords> bfpt_dwords{};
  uint32_t erase_timing_dword_raw{0U};
  uint32_t erase_to_suspend_resume_dword_raw{0U};
  uint32_t program_timing_dword_raw{0U};
};

/**
 * @brief JEDEC SPI NOR command set.
 */
struct Commands {
  static constexpr uint8_t kReadDataOpcode{0x03U};
  static constexpr uint8_t kPageProgramOpcode{0x02U};
  static constexpr uint8_t kWriteEnableOpcode{0x06U};
  static constexpr uint8_t kWriteDisableOpcode{0x04U};
  static constexpr uint8_t kReadStatusRegister1Opcode{0x05U};
  static constexpr uint8_t kSectorEraseOpcode{0x20U};
  static constexpr uint8_t kChipEraseOpcode{0xC7U};
  static constexpr uint8_t kReadJedecIdOpcode{0x9FU};

  uint8_t read_data{kReadDataOpcode};          ///< Read data opcode.
  uint8_t page_program{kPageProgramOpcode};    ///< Page program opcode.
  uint8_t write_enable{kWriteEnableOpcode};    ///< Write enable opcode.
  uint8_t write_disable{kWriteDisableOpcode};  ///< Write disable opcode.
  uint8_t read_status_register_1{
      kReadStatusRegister1Opcode};           ///< Status register 1 read opcode.
  uint8_t sector_erase{kSectorEraseOpcode};  ///< Typical 4KB erase opcode.
  uint8_t chip_erase{kChipEraseOpcode};      ///< Full-chip erase opcode.
  uint8_t read_jedec_id{kReadJedecIdOpcode};  ///< JEDEC ID read opcode.
};

}  // namespace sfw::device::jedec_flash

#endif  // DEVICE_JEDEC_FLASH_JEDEC_FLASH_TYPES_HPP