// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef DEVICE_JEDEC_FLASH_JEDEC_SPI_FLASH_HPP
#define DEVICE_JEDEC_FLASH_JEDEC_SPI_FLASH_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "device/jedec_flash/jedec_flash_types.hpp"
#include "hal_interface/digital_output.hpp"
#include "hal_interface/error_code.hpp"
#include "hal_interface/software_timer.hpp"
#include "hal_interface/spi_controller.hpp"

namespace sfw::device::jedec_flash {

/**
 * @brief Low-level JEDEC-compatible SPI NOR flash access.
 *
 * This class sends standard JEDEC SPI NOR opcodes through an injected
 * SpiController and chip-select DigitalOutput. It is transport-oriented and
 * does not enforce memory geometry constraints such as page alignment or
 * sector layout.
 *
 * Include path:
 * #include "device/jedec_flash/jedec_spi_flash.hpp"
 *
 * Typical usage:
 * 1. Construct this class with a SpiController and CS DigitalOutput.
 * 2. Call Initialize() once before issuing flash commands.
 * 3. Call ReadJedecId() to confirm the connected memory target.
 * 4. Use ReadData(), ProgramPage(), EraseSector(), and EraseChip().
 * 5. Call WaitUntilReady() after mutating commands when needed.
 * 6. Call Deinitialize() when this access layer is no longer used.
 */
class JedecSpiFlash final {
 public:
  /**
   * @brief Constructs a JEDEC SPI flash command transport.
   *
   * @param[in] spi SPI controller used for all transactions.
   * @param[in] cs_pin Chip-select pin used for all transactions.
   * @param[in] cs_active_state Logical level used to assert chip-select.
   * @param[in] timer Software timer used for busy-timeout handling.
   */
  JedecSpiFlash(hal_interface::SpiController& spi,
                hal_interface::DigitalOutput& cs_pin, bool cs_active_state,
                hal_interface::SoftwareTimer& timer);

  /**
   * @brief Constructs a JEDEC SPI flash command transport with custom opcodes.
   *
   * @p command_set allows overriding the standard JEDEC opcode values, which
   * may be useful for supporting non-standard or vendor-specific commands.
   *
   * @param[in] spi SPI controller used for all transactions.
   * @param[in] cs_pin Chip-select pin used for all transactions.
   * @param[in] cs_active_state Logical level used to assert chip-select.
   * @param[in] timer Software timer used for busy-timeout handling.
   * @param[in] command_set JEDEC opcode configuration.
   */
  JedecSpiFlash(hal_interface::SpiController& spi,
                hal_interface::DigitalOutput& cs_pin, bool cs_active_state,
                hal_interface::SoftwareTimer& timer,
                const jedec_flash::Commands& command_set);

  JedecSpiFlash(const JedecSpiFlash&) = delete;
  JedecSpiFlash& operator=(const JedecSpiFlash&) = delete;
  JedecSpiFlash(JedecSpiFlash&&) = delete;
  JedecSpiFlash& operator=(JedecSpiFlash&&) = delete;

  /**
   * @brief Destructor
   */
  ~JedecSpiFlash();

  /**
   * @brief Initializes injected SPI, CS, and timer dependencies.
   *
   * @retval hal_interface::ErrorCode::kOk Dependencies are initialized.
   * @retval hal_interface::ErrorCode::kError Dependency initialization failed.
   */
  hal_interface::ErrorCode Initialize();

  /**
   * @brief Deinitializes this class and all injected dependencies.
   *
   * Since the SPI controller might be shared with other devices, this method
   * does not deinitialize it.
   *
   * @retval hal_interface::ErrorCode::kOk Deinitialization completed.
   * @retval hal_interface::ErrorCode::kError One or more dependency teardown
   * operations failed.
   */
  hal_interface::ErrorCode Deinitialize();

  /**
   * @brief Returns whether this class has been initialized.
   *
   * @retval true This class is initialized.
   * @retval false This class is not initialized.
   */
  [[nodiscard]] bool IsInitialized() const;

  /**
   * @brief Reads the 3-byte JEDEC manufacturer and device identifier.
   *
   * @param[out] jedec_id Destination array for the JEDEC identifier bytes.
   * @param[in] timeout_ms Maximum time to wait for the transfer.
   * @retval hal_interface::ErrorCode::kOk JEDEC identifier read completed.
   * @retval hal_interface::ErrorCode::kInvalidArgument @p timeout_ms is zero.
   * @retval hal_interface::ErrorCode::kError Class is not initialized or I/O
   * failed.
   */
  hal_interface::ErrorCode ReadJedecId(std::span<uint8_t, 3> jedec_id,
                                       uint32_t timeout_ms);

  /**
   * @brief Reads JEDEC-compatible discovery information.
   *
   * This method always attempts to read the JEDEC ID first. It then reads SFDP
   * data to populate density, addressing capability and timing-related BFPT
   * fields.
   *
   * @param[out] info Output structure populated by this method.
   * @param[in] timeout_ms Maximum time to wait for each SPI transaction.
   * @retval hal_interface::ErrorCode::kOk Discovery completed.
   * @retval hal_interface::ErrorCode::kInvalidArgument Invalid arguments.
   * @retval hal_interface::ErrorCode::kNotSupported SFDP is not available.
   * @retval hal_interface::ErrorCode::kError Class is not initialized or I/O
   * failed.
   */
  hal_interface::ErrorCode ReadJedecInfo(jedec_flash::Info& info,
                                         uint32_t timeout_ms);

  /**
   * @brief Reads data bytes from the target flash.
   *
   * @param[in] address Start address to read from.
   * @param[in] address_size_bytes Number of address bytes in commands.
   * The SpiController register API supports command+address fields up to
   * 4 bytes total, so @p address_size_bytes must be 1 to 3.
   * @param[out] buffer Destination span to receive bytes.
   * @param[in] timeout_ms Maximum time to wait for the transfer.
   * @retval hal_interface::ErrorCode::kOk Read completed.
   * @retval hal_interface::ErrorCode::kInvalidArgument Invalid arguments.
   * @retval hal_interface::ErrorCode::kError Class is not initialized or I/O
   * failed.
   */
  hal_interface::ErrorCode ReadData(uint32_t address,
                                    uint8_t address_size_bytes,
                                    std::span<uint8_t> buffer,
                                    uint32_t timeout_ms);

  /**
   * @brief Programs one page chunk at the provided address.
   *
   * This method does not check page boundaries. The caller must split writes
   * according to the target geometry.
   *
   * @param[in] address Start address to program.
   * @param[in] address_size_bytes Number of address bytes in commands.
   * The SpiController register API supports command+address fields up to
   * 4 bytes total, so @p address_size_bytes must be 1 to 3.
   * @param[in] data Source bytes to program.
   * @param[in] timeout_ms Maximum time to wait for the transfer.
   * @retval hal_interface::ErrorCode::kOk Program command completed.
   * @retval hal_interface::ErrorCode::kInvalidArgument Invalid arguments.
   * @retval hal_interface::ErrorCode::kTimeout Device stayed busy too long.
   * @retval hal_interface::ErrorCode::kError Class is not initialized or I/O
   * failed.
   */
  hal_interface::ErrorCode ProgramPage(uint32_t address,
                                       uint8_t address_size_bytes,
                                       std::span<const uint8_t> data,
                                       uint32_t timeout_ms);

  /**
   * @brief Erases one sector containing @p address.
   *
   * @param[in] address Target sector base or in-sector address.
   * @param[in] address_size_bytes Number of address bytes in commands.
   * The SpiController register API supports command+address fields up to
   * 4 bytes total, so @p address_size_bytes must be 1 to 3.
   * @param[in] timeout_ms Maximum time to wait for completion.
   * @retval hal_interface::ErrorCode::kOk Sector erase completed.
   * @retval hal_interface::ErrorCode::kInvalidArgument Invalid arguments.
   * @retval hal_interface::ErrorCode::kTimeout Device stayed busy too long.
   * @retval hal_interface::ErrorCode::kError Class is not initialized or I/O
   * failed.
   */
  hal_interface::ErrorCode EraseSector(uint32_t address,
                                       uint8_t address_size_bytes,
                                       uint32_t timeout_ms);

  /**
   * @brief Erases the whole flash memory.
   *
   * @param[in] timeout_ms Maximum time to wait for completion.
   * @retval hal_interface::ErrorCode::kOk Full erase completed.
   * @retval hal_interface::ErrorCode::kInvalidArgument Invalid arguments.
   * @retval hal_interface::ErrorCode::kTimeout Device stayed busy too long.
   * @retval hal_interface::ErrorCode::kError Class is not initialized or I/O
   * failed.
   */
  hal_interface::ErrorCode EraseChip(uint32_t timeout_ms);

  /**
   * @brief Waits until the device busy flag clears.
   *
   * @param[in] timeout_ms Maximum busy-polling time.
   * @retval hal_interface::ErrorCode::kOk Device became ready.
   * @retval hal_interface::ErrorCode::kInvalidArgument @p timeout_ms is zero.
   * @retval hal_interface::ErrorCode::kTimeout Busy flag did not clear.
   * @retval hal_interface::ErrorCode::kError Status reads failed.
   */
  hal_interface::ErrorCode WaitUntilReady(uint32_t timeout_ms);

 private:
  hal_interface::ErrorCode SetWriteEnable(bool enable, uint32_t timeout_ms);
  hal_interface::ErrorCode ReadStatusRegister1(uint8_t& status,
                                               uint32_t timeout_ms);
  hal_interface::ErrorCode ReadSfdpBytes(uint32_t sfdp_address,
                                         std::span<uint8_t> buffer,
                                         uint32_t timeout_ms);
  hal_interface::ErrorCode FindBfptTable(uint8_t parameter_header_count,
                                         uint32_t timeout_ms,
                                         uint8_t& bfpt_dword_count_available,
                                         uint32_t& bfpt_pointer);
  hal_interface::ErrorCode ReadBfptTable(uint32_t bfpt_pointer,
                                         uint8_t bfpt_dword_count_available,
                                         uint32_t timeout_ms,
                                         jedec_flash::Info& info);
  static void PopulateInfoFromBfpt(jedec_flash::Info& info);
  [[nodiscard]] static bool IsAddressSizeSupported(uint8_t address_size_bytes);
  [[nodiscard]] static uint32_t BuildCommandAddress(uint8_t opcode,
                                                    uint32_t address,
                                                    uint8_t address_size_bytes);
  [[nodiscard]] static uint32_t DecodeLe32(std::span<const uint8_t, 4> bytes);
  [[nodiscard]] static uint32_t DecodeLe24(std::span<const uint8_t, 3> bytes);

  hal_interface::SpiController& spi_;
  hal_interface::DigitalOutput& cs_pin_;
  bool cs_active_state_;
  hal_interface::SoftwareTimer& timer_;
  jedec_flash::Commands command_set_;
  bool initialized_{false};

  static constexpr uint8_t kStatusBusyMask{0x01U};

  static constexpr uint8_t kSfdpReadOpcode{0x5AU};
  static constexpr uint32_t kSfdpSignature{0x50444653U};
  static constexpr uint8_t kSfdpHeaderSizeBytes{8U};
  static constexpr uint8_t kSfdpParameterHeaderSizeBytes{8U};
  static constexpr uint8_t kSfdpReadPrefixSizeBytes{5U};
  static constexpr uint8_t kSfdpHeaderMinorRevisionIndex{4U};
  static constexpr uint8_t kSfdpHeaderMajorRevisionIndex{5U};
  static constexpr uint8_t kSfdpHeaderParameterCountIndex{6U};
  static constexpr uint8_t kSfdpParameterIdLsbIndex{0U};
  static constexpr uint8_t kSfdpParameterLengthIndex{3U};
  static constexpr uint8_t kSfdpParameterPointerIndex{4U};
  static constexpr uint8_t kSfdpParameterIdMsbIndex{7U};
  static constexpr uint8_t kBytesPerDword{4U};
  static constexpr uint8_t kShift8Bits{8U};
  static constexpr uint8_t kShift16Bits{16U};
  static constexpr uint8_t kShift24Bits{24U};
  static constexpr uint8_t kShift32Bits{32U};
  static constexpr uint32_t kByteMask{0xFFU};
  static constexpr uint8_t kBfptIdLsb{0x00U};
  static constexpr uint8_t kBfptIdMsb{0xFFU};
  static constexpr uint8_t kMaxBfptDwordsToRead{
      static_cast<uint8_t>(jedec_flash::Info::kMaxBfptDwords)};
  static constexpr uint8_t kBitsPerByte{8U};
  static constexpr uint8_t kBfptDwordDensityIndex{1U};
  static constexpr uint8_t kBfptDwordAddressingIndex{0U};
  static constexpr uint8_t kBfptDwordEraseTimingIndex{9U};
  static constexpr uint8_t kBfptDwordEraseSuspendResumeIndex{11U};
  static constexpr uint8_t kBfptDwordProgramTimingIndex{10U};
  static constexpr uint8_t kAddressSupportBitOffset{17U};
  static constexpr uint32_t kAddressSupportBitMask{0x03U};
  static constexpr uint32_t kDensityLargeFlag{0x80000000U};
  static constexpr uint32_t kDensityExponentMask{0x7FFFFFFFU};
  static constexpr uint8_t kMaxSafeShiftBits{63U};
  static constexpr uint8_t kMinAddressSizeBytes{1U};
  static constexpr uint8_t kMaxAddressSizeBytes{3U};
  static constexpr uint8_t kSfdpTxAddressMsbIndex{1U};
  static constexpr uint8_t kSfdpTxAddressMidIndex{2U};
  static constexpr uint8_t kSfdpTxAddressLsbIndex{3U};
  static constexpr uint8_t kSfdpTxDummyByteIndex{4U};
  static constexpr uint32_t kBusyPollDelayMs{1U};
  static constexpr uint32_t kControlOpTimeoutMs{100U};
};

}  // namespace sfw::device::jedec_flash

#endif  // DEVICE_JEDEC_FLASH_JEDEC_SPI_FLASH_HPP
