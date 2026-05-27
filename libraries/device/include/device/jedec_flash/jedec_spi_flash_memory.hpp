// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef DEVICE_JEDEC_FLASH_JEDEC_SPI_FLASH_MEMORY_HPP
#define DEVICE_JEDEC_FLASH_JEDEC_SPI_FLASH_MEMORY_HPP

#include <cstdint>
#include <span>

#include "device/jedec_flash/jedec_spi_flash.hpp"
#include "hal_interface/error_code.hpp"
#include "hal_interface/memory.hpp"

namespace sfw::device::jedec_flash {

/**
 * @brief Memory adapter for JEDEC SPI NOR flash devices.
 *
 * This class implements hal_interface::Memory and delegates low-level SPI
 * flash transactions to JedecSpiFlash.
 * Geometry, alignment, and chunking behavior are controlled by injected
 * MemoryMetadata.
 *
 * Include path:
 * #include "device/jedec_flash/jedec_spi_flash_memory.hpp"
 *
 * Typical usage:
 * 1. Create a JedecSpiFlash with SPI and CS dependencies.
 * 2. (Optional)Define MemoryMetadata for the concrete flash geometry.
 * 3. Construct JedecSpiFlashMemory with the device and metadata.
 * 4. Call Initialize() once.
 * 5. Use Read(), Write(), EraseBlock(), and EraseAllMemory().
 * 6. Call Deinitialize() when memory operations are no longer needed.
 */
class JedecSpiFlashMemory final : public hal_interface::Memory {
 public:
  /**
   * @brief Constructs a JEDEC SPI flash memory adapter.
   *
   * If this constructor is used, the adapter will attempt to populate metadata
   * by reading the flash device's identification and configuration registers.
   *
   * @param[in] flash_device Low-level JEDEC SPI flash transport.
   */
  explicit JedecSpiFlashMemory(JedecSpiFlash& flash_device);

  /**
   * @brief Constructs a JEDEC SPI flash memory adapter.
   *
   * @param[in] flash_device Low-level JEDEC SPI flash transport.
   * @param[in] metadata Geometry and timing metadata for this memory target.
   */
  JedecSpiFlashMemory(JedecSpiFlash& flash_device,
                      const hal_interface::MemoryMetadata& metadata);

  JedecSpiFlashMemory(const JedecSpiFlashMemory&) = delete;
  JedecSpiFlashMemory& operator=(const JedecSpiFlashMemory&) = delete;
  JedecSpiFlashMemory(JedecSpiFlashMemory&&) = delete;
  JedecSpiFlashMemory& operator=(JedecSpiFlashMemory&&) = delete;

  /**
   * @brief Destructor
   */
  ~JedecSpiFlashMemory() override;

  /**
   * @brief Initializes the low-level flash transport.
   *
   * If metadata was not provided at construction, this method also attempts to
   * populate it by reading the flash device's identification and configuration
   * registers.
   *
   * @retval hal_interface::ErrorCode::kOk Initialization completed.
   * @retval hal_interface::ErrorCode::kError Dependency initialization failed.
   */
  hal_interface::ErrorCode Initialize() override;

  /**
   * @brief Deinitializes this memory adapter.
   *
   * @retval hal_interface::ErrorCode::kOk Deinitialization completed.
   * @retval hal_interface::ErrorCode::kError Dependency deinitialization
   * failed.
   */
  hal_interface::ErrorCode Deinitialize() override;

  /**
   * @brief Returns whether this adapter is initialized.
   *
   * @retval true Adapter is initialized.
   * @retval false Adapter is not initialized.
   */
  bool IsInitialized() override;

  /**
   * @brief Reads bytes from the configured memory region.
   *
   * @param[in] start_address Absolute start address in memory space.
   * @param[out] buffer Destination span to receive bytes.
   * @param[in] timeout_ms Maximum time for each transfer chunk.
   * @retval hal_interface::ErrorCode::kOk Read completed.
   * @retval hal_interface::ErrorCode::kTimeout Read exceeded timeout.
   * @retval hal_interface::ErrorCode::kError Invalid range or I/O failure.
   */
  hal_interface::ErrorCode Read(uint64_t start_address,
                                std::span<uint8_t> buffer,
                                uint32_t timeout_ms) override;

  /**
   * @brief Writes bytes to flash respecting metadata chunking rules.
   *
   * @param[in] start_address Absolute start address in memory space.
   * @param[in] buffer Source bytes to write.
   * @param[in] timeout_ms Maximum time for each transfer chunk.
   * @retval hal_interface::ErrorCode::kOk Write completed.
   * @retval hal_interface::ErrorCode::kTimeout Write exceeded timeout.
   * @retval hal_interface::ErrorCode::kError Invalid range, alignment, or
   * I/O failure.
   */
  hal_interface::ErrorCode Write(uint64_t start_address,
                                 std::span<const uint8_t> buffer,
                                 uint32_t timeout_ms) override;

  /**
   * @brief Erases the sector containing @p address_within_sector.
   *
   * @param[in] address_within_sector Address located in the target sector.
   * @param[in] timeout_ms Maximum time for the erase command.
   * @retval hal_interface::ErrorCode::kOk Sector erase completed.
   * @retval hal_interface::ErrorCode::kTimeout Erase exceeded timeout.
   * @retval hal_interface::ErrorCode::kError Invalid range or I/O failure.
   */
  hal_interface::ErrorCode EraseBlock(uint64_t address_within_sector,
                                      uint32_t timeout_ms) override;

  /**
   * @brief Erases the whole memory device.
   *
   * @param[in] timeout_ms Maximum time for the erase command.
   * @retval hal_interface::ErrorCode::kOk Full erase completed.
   * @retval hal_interface::ErrorCode::kTimeout Erase exceeded timeout.
   * @retval hal_interface::ErrorCode::kError I/O failure.
   */
  hal_interface::ErrorCode EraseAllMemory(uint32_t timeout_ms) override;

  /**
   * @brief Returns immutable metadata configured for this memory instance.
   *
   * @retval hal_interface::MemoryMetadata Metadata copy.
   */
  [[nodiscard]] hal_interface::MemoryMetadata GetMetadata() const override;

 private:
  struct EraseTypeSelection {
    uint8_t erase_type_index{0U};
    uint32_t size_bytes{0U};
  };

  [[nodiscard]] bool IsRangeValid(uint64_t start_address,
                                  uint64_t size_bytes) const;
  [[nodiscard]] hal_interface::ErrorCode TranslateAddress(
      uint64_t relative_address, uint32_t& device_address) const;
  [[nodiscard]] uint32_t ResolveReadChunkSize(uint64_t remaining) const;
  [[nodiscard]] uint32_t ResolveWriteChunkSize(uint64_t current_address,
                                               uint64_t remaining) const;
  [[nodiscard]] bool PopulateMetadataFromJedecInfo();

  [[nodiscard]] static uint32_t SaturateToU32(uint64_t value);
  [[nodiscard]] static uint32_t DecodeTypicalEraseTimeMs(
      uint32_t erase_timing_dword, uint8_t erase_type_index);
  [[nodiscard]] static uint32_t DecodeTypicalToMaxFactor(uint32_t dword);
  [[nodiscard]] static uint32_t DecodeMaximumSectorEraseTimeMs(
      uint32_t erase_timing, uint8_t erase_type_idx);
  [[nodiscard]] static uint32_t DecodeMaximumWriteTimeUs(
      uint32_t program_timing_dword);
  [[nodiscard]] static uint32_t DecodeMaximumChipEraseTimeMs(
      uint32_t program_timing_dword);
  [[nodiscard]] static uint32_t DecodeEraseTypeSizeBytes(uint32_t dword,
                                                         uint8_t opcode_shift,
                                                         uint8_t size_shift);
  [[nodiscard]] static EraseTypeSelection DecodeSmallestEraseType(
      const jedec_flash::Info& info);
  [[nodiscard]] static uint32_t DecodePageSizeBytes(
      const jedec_flash::Info& info);

  JedecSpiFlash& flash_device_;
  hal_interface::MemoryMetadata metadata_{};
  bool has_user_metadata_;
  bool initialized_{false};

  static constexpr uint8_t kAddressSizeBytes{3U};
  static constexpr uint32_t kMetadataReadTimeoutMs{100U};
  static constexpr uint8_t kBfptDwordEraseType12Index{7U};
  static constexpr uint8_t kBfptDwordEraseType34Index{8U};
  static constexpr uint8_t kEraseType1SizeShift{0U};
  static constexpr uint8_t kEraseType2SizeShift{16U};
  static constexpr uint8_t kEraseType1OpcodeShift{8U};
  static constexpr uint8_t kEraseType2OpcodeShift{24U};
  static constexpr uint32_t kOneByteValue{1U};
  static constexpr uint32_t kMetadataByteMask{0xFFU};
  static constexpr uint8_t kPageSizeExponentShift{4U};
  static constexpr uint32_t kPageSizeExponentMask{0x0FU};
  static constexpr uint8_t kMaxSafeShiftBitsU32{31U};
  static constexpr uint8_t kEraseTypeTimeFieldWidthBits{7U};
  static constexpr uint8_t kEraseTypeTimeCountWidthBits{5U};
  static constexpr uint8_t kEraseTypeTimeEntriesBaseShift{4U};
  static constexpr uint8_t kEraseTypeCountMask{0x1FU};
  static constexpr uint8_t kEraseTypeUnitMask{0x03U};
  static constexpr uint8_t kMaxFactorMask{0x0FU};
  static constexpr uint8_t kChipEraseTimeCountShift{24U};
  static constexpr uint8_t kChipEraseTimeUnitShift{29U};
  static constexpr uint8_t kPageProgramTimeCountShift{8U};
  static constexpr uint8_t kChipEraseTimeCountMask{0x1FU};
  static constexpr uint8_t kPageProgramTimeCountMask{0x1FU};
  static constexpr uint8_t kPageProgramScaleBit{13U};
  static constexpr uint64_t kMsPerSecond{1000U};
  static constexpr uint64_t kEraseTimeUnit16Ms{16U};
  static constexpr uint64_t kEraseTimeUnit128Ms{128U};
  static constexpr uint64_t kChipEraseTimeUnit256Ms{256U};
  static constexpr uint64_t kPageProgramScaleSmallUs{8U};
  static constexpr uint64_t kPageProgramScaleLargeUs{64U};
  static constexpr uint64_t kChipEraseTimeUnit4SInMs{4U * kMsPerSecond};
  static constexpr uint64_t kChipEraseTimeUnit64SInMs{64U * kMsPerSecond};
};

}  // namespace sfw::device::jedec_flash

#endif  // DEVICE_JEDEC_FLASH_JEDEC_SPI_FLASH_MEMORY_HPP
