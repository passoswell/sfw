// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef DEVICE_SERIAL_EEPROM_SERIAL_EEPROM_HPP
#define DEVICE_SERIAL_EEPROM_SERIAL_EEPROM_HPP

#include <cstdint>
#include <span>

#include "device/serial_eeprom/interface/serial_eeprom_transport_interface.hpp"
#include "hal_interface/error_code.hpp"
#include "hal_interface/memory.hpp"

namespace sfw::device::serial_eeprom {

/**
 * @brief Generic serial EEPROM memory adapter.
 *
 * This class implements hal_interface::Memory and uses an injected transport
 * plus injected MemoryMetadata to support multiple serial EEPROM families.
 *
 * Include path:
 * #include "device/serial_eeprom/serial_eeprom.hpp"
 *
 * Typical usage:
 * 1. Create an I2C transport for the EEPROM target.
 * 2. Define MemoryMetadata for the selected EEPROM geometry.
 * 3. Construct SerialEeprom with transport and metadata.
 * 4. Call Initialize().
 * 5. Use Read()/Write() through hal_interface::Memory.
 * 6. Call Deinitialize() when memory access is no longer needed.
 */
class SerialEeprom final : public hal_interface::Memory {
 public:
  /**
   * @brief Constructs a generic serial EEPROM adapter.
   *
   * @param[in] transport Transport used for low-level bus transfers.
   * @param[in] metadata EEPROM geometry and timing metadata.
   */
  SerialEeprom(SerialEepromTransportInterface& transport,
               const hal_interface::MemoryMetadata& metadata,
               uint8_t erased_value = 0xFFU);  // NOLINT

  SerialEeprom(const SerialEeprom&) = delete;
  SerialEeprom& operator=(const SerialEeprom&) = delete;
  SerialEeprom(SerialEeprom&&) = delete;
  SerialEeprom& operator=(SerialEeprom&&) = delete;

  /**
   * @brief Destructor
   */
  ~SerialEeprom() override = default;

  /**
   * @brief Initializes the EEPROM adapter.
   *
   * Validates address size and metadata, then initializes the injected
   * transport.
   *
   * @retval hal_interface::ErrorCode::kOk Initialization succeeded.
   * @retval hal_interface::ErrorCode::kInvalidArgument Configuration is
   * invalid.
   * @retval hal_interface::ErrorCode::kError Transport initialization failed.
   */
  hal_interface::ErrorCode Initialize() override;

  /**
   * @brief Deinitializes the EEPROM adapter.
   *
   * Deinitializes the injected transport and clears this class state.
   *
   * @retval hal_interface::ErrorCode::kOk Deinitialization succeeded.
   * @retval hal_interface::ErrorCode::kError Transport deinitialization
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
   * @brief Reads bytes from EEPROM memory space.
   *
   * Splits the read into transport transactions according to configured
   * read block size.
   *
   * @param[in] start_address Relative start address inside the memory region.
   * @param[out] buffer Destination span that receives the read bytes.
   * @param[in] timeout_ms Maximum time for each transfer, in milliseconds.
   * @retval hal_interface::ErrorCode::kOk Read completed.
   * @retval hal_interface::ErrorCode::kInvalidArgument Invalid timeout.
   * @retval hal_interface::ErrorCode::kOutOfRange Address range is invalid.
   * @retval hal_interface::ErrorCode::kTimeout Transfer timed out.
   * @retval hal_interface::ErrorCode::kError Transport or state error.
   */
  hal_interface::ErrorCode Read(uint64_t start_address,
                                std::span<uint8_t> buffer,
                                uint32_t timeout_ms) override;

  /**
   * @brief Writes bytes to EEPROM memory space.
   *
   * Splits the write into page-constrained transactions and waits for ready
   * status after each write cycle.
   *
   * @param[in] start_address Relative start address inside the memory region.
   * @param[in] buffer Source bytes to write.
   * @param[in] timeout_ms Maximum time for each transfer, in milliseconds.
   * @retval hal_interface::ErrorCode::kOk Write completed.
   * @retval hal_interface::ErrorCode::kInvalidArgument Invalid timeout.
   * @retval hal_interface::ErrorCode::kOutOfRange Address range is invalid.
   * @retval hal_interface::ErrorCode::kTimeout Transfer timed out.
   * @retval hal_interface::ErrorCode::kError Alignment, transport, or state
   * error.
   */
  hal_interface::ErrorCode Write(uint64_t start_address,
                                 std::span<const uint8_t> buffer,
                                 uint32_t timeout_ms) override;

  /**
   * @brief Emulates a block erase by writing erased values.
   *
   * Serial EEPROM devices do not provide native block erase commands. This
   * method writes the erased value over one configured erase block.
   * This method creates an internal buffer of size kEraseBufferSizeBytes to
   * perform the block erase in chunks. If the erase block size is smaller than
   * this buffer, the method will still work with the same amount of memory
   * allocated.
   *
   * @param[in] address_within_sector Address located in the target block.
   * @param[in] timeout_ms Maximum time for each transfer, in milliseconds.
   * @retval hal_interface::ErrorCode::kOk Block erase emulation completed.
   * @retval hal_interface::ErrorCode::kInvalidArgument Invalid timeout.
   * @retval hal_interface::ErrorCode::kNotSupported Erase block size is zero.
   * @retval hal_interface::ErrorCode::kOutOfRange Address range is invalid.
   * @retval hal_interface::ErrorCode::kTimeout Transfer timed out.
   * @retval hal_interface::ErrorCode::kError Transport or state error.
   */
  hal_interface::ErrorCode EraseBlock(uint64_t address_within_sector,
                                      uint32_t timeout_ms) override;

  /**
   * @brief Emulates full-memory erase by writing erased values.
   *
   * Serial EEPROM devices do not provide native chip erase commands. This
   * method writes the erased value across the whole configured memory region.
   *
   * @param[in] timeout_ms Maximum time for each transfer, in milliseconds.
   * @retval hal_interface::ErrorCode::kOk Full erase emulation completed.
   * @retval hal_interface::ErrorCode::kInvalidArgument Invalid timeout.
   * @retval hal_interface::ErrorCode::kNotSupported Memory size unsupported
   * by the host.
   * @retval hal_interface::ErrorCode::kOutOfRange Address range is invalid.
   * @retval hal_interface::ErrorCode::kTimeout Transfer timed out.
   * @retval hal_interface::ErrorCode::kError Transport or state error.
   */
  hal_interface::ErrorCode EraseAllMemory(uint32_t timeout_ms) override;

  /**
   * @brief Returns immutable metadata for this EEPROM instance.
   *
   * @retval hal_interface::MemoryMetadata Metadata copy.
   */
  [[nodiscard]] hal_interface::MemoryMetadata GetMetadata() const override;

 private:
  [[nodiscard]] bool IsRangeValid(uint64_t start_address,
                                  uint64_t size_bytes) const;
  [[nodiscard]] hal_interface::ErrorCode EncodeAddress(
      uint64_t relative_address, uint32_t& output) const;
  [[nodiscard]] uint32_t ResolveReadChunkSize(uint64_t remaining) const;
  [[nodiscard]] uint32_t ResolveWriteChunkSize(uint64_t current_address,
                                               uint64_t remaining) const;

  SerialEepromTransportInterface& transport_;
  hal_interface::MemoryMetadata metadata_{};
  uint8_t erased_value_{kDefaultErasedValue};
  bool initialized_{false};

  static constexpr std::size_t kMaxAddressSizeBytes{sizeof(uint32_t)};
  static constexpr uint8_t kBitsPerByte{8U};
  static constexpr uint8_t kMaxAddressBits{32U};
  static constexpr uint8_t kDefaultErasedValue{0xFFU};
  static constexpr std::size_t kEraseBufferSizeBytes{32U};
};

}  // namespace sfw::device::serial_eeprom

#endif  // DEVICE_SERIAL_EEPROM_SERIAL_EEPROM_HPP
