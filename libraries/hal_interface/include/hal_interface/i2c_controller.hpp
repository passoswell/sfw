// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_INTERFACE_I2C_CONTROLLER_HPP
#define HAL_INTERFACE_I2C_CONTROLLER_HPP

#include <cstdint>
#include <span>

#include "hal_interface/error_code.hpp"

namespace sfw::hal_interface {

/**
 * @brief Abstract interface for an I2C controller.
 *
 * Provides methods for raw byte transfers as well as register-addressed read
 * and write operations commonly used when communicating with devices such as
 * EEPROMs.
 * An I2C controller is the peripheral/device responsible for initiating the
 * communication on the bus, while the targets are the devices being addressed.
 * The target device address, clock speed and other parameters are assumed to be
 * configured at construction time by the concrete implementation. All the
 * methods on this interface are synchronous / blocking. This means they only
 * return if the operation is finished or if an error occurred.
 *
 * Typical usage:
 * 1. Call Initialize() once to configure the I2C hardware (clock speed, GPIO
 * pins, etc.).
 * 2. Call Read(), Write(), or WriteThenRead() for raw byte transfers.
 * 3. Call ReadRegister() or WriteRegister() for register-based operations with
 * I2C devices.
 * 4. Handle error codes such as kTimeout or kError for communication failures.
 * 5. Call Deinitialize() when the peripheral is no longer needed.
 */
class I2cController {
 public:
  I2cController() = default;
  I2cController(const I2cController&) = default;
  I2cController& operator=(const I2cController&) = default;
  I2cController(I2cController&&) = default;
  I2cController& operator=(I2cController&&) = default;

  /**
   * @brief Destructor
   */
  virtual ~I2cController() = default;

  /**
   * @brief Initializes the I2C controller peripheral.
   *
   * Configures the I2C hardware (clock speed, GPIO pins, etc.). These
   * parameters, if needed, must be passed through the constructor of the
   * concrete implementation. Must be called before any transfer method.
   *
   * @retval ErrorCode::kOk    Initialization succeeded.
   * @retval ErrorCode::kError Initialization failed due to a hardware fault.
   */
  virtual ErrorCode Initialize() = 0;

  /**
   * @brief Deinitializes the I2C controller peripheral.
   *
   * Releases resources allocated by Initialize(). After this call, transfer
   * methods must fail until Initialize() is called again.
   * Registers are deinitialized and peripherals can be put in low power or
   * off mode.
   *
   * @retval ErrorCode::kOk Peripheral deinitialized successfully.
   * @retval ErrorCode::kError Deinitialization failed.
   */
  virtual ErrorCode Deinitialize() = 0;

  /**
   * @brief Returns whether the peripheral has been successfully initialized.
   *
   * @retval true  Initialize() has been called and succeeded.
   * @retval false The peripheral is not yet initialized.
   */
  virtual bool IsInitialized() = 0;

  /**
   * @brief Reads bytes from the target device into @p buffer.
   *
   * Generates a START condition, transmits the device address with the read
   * bit set, receives @p buffer.size() bytes, and generates a STOP condition.
   *
   * @param[in] target_address 7-bit or 10-bit I2C target address.
   * @param[in] is_10bit_address Whether the target address is 10-bit (true) or
   * 7-bit (false).
   * @param[out] buffer     Destination span that receives the read bytes.
   * @param[in]  timeout_ms Maximum time to wait for the transfer, in
   * milliseconds.
   * @retval ErrorCode::kOk      All bytes were read successfully.
   * @retval ErrorCode::kTimeout The transfer did not complete within @p
   * timeout_ms.
   * @retval ErrorCode::kError   A bus error or NACK was detected.
   */
  virtual ErrorCode Read(uint16_t target_address, bool is_10bit_address,
                         std::span<uint8_t> buffer, uint32_t timeout_ms) = 0;

  /**
   * @brief Writes bytes from @p buffer to the target device.
   *
   * Generates a START condition, transmits the device address with the write
   * bit set, sends all bytes in @p buffer, and generates a STOP condition.
   *
   * @param[in] target_address 7-bit or 10-bit I2C target address.
   * @param[in] is_10bit_address Whether the target address is 10-bit (true) or
   * 7-bit (false).
   * @param[in] buffer     Source span containing the bytes to transmit.
   * @param[in] timeout_ms Maximum time to wait for the transfer, in
   * milliseconds.
   * @retval ErrorCode::kOk      All bytes were written successfully.
   * @retval ErrorCode::kTimeout The transfer did not complete within @p
   * timeout_ms.
   * @retval ErrorCode::kError   A bus error or NACK was detected.
   */
  virtual ErrorCode Write(uint16_t target_address, bool is_10bit_address,
                          std::span<const uint8_t> buffer,
                          uint32_t timeout_ms) = 0;

  /**
   * @brief Performs a combined write-then-read transaction without releasing
   * the bus.
   *
   * Sends all bytes in @p write_buffer, issues a repeated START, then reads
   * @p read_buffer.size() bytes. The bus is released with a STOP condition
   * after the read phase. This pattern is commonly used to write a command or
   * address and immediately read back the result.
   *
   * @param[in] target_address 7-bit or 10-bit I2C target address.
   * @param[in] is_10bit_address Whether the target address is 10-bit (true) or
   * 7-bit (false).
   * @param[in]  write_buffer Source span containing the bytes to transmit.
   * @param[out] read_buffer  Destination span that receives the read bytes.
   * @param[in]  timeout_ms   Maximum time to wait for the entire transaction,
   * in milliseconds.
   * @retval ErrorCode::kOk      The transaction completed successfully.
   * @retval ErrorCode::kTimeout The transaction did not complete within @p
   * timeout_ms.
   * @retval ErrorCode::kError   A bus error or NACK was detected.
   */
  virtual ErrorCode WriteThenRead(uint16_t target_address,
                                  bool is_10bit_address,
                                  std::span<const uint8_t> write_buffer,
                                  std::span<uint8_t> read_buffer,
                                  uint32_t timeout_ms) = 0;

  /**
   * @brief Reads bytes from a specific register address of the target device.
   *
   * Transmits the register address (MSB first, @p register_size_bytes wide),
   * issues a repeated START, then reads @p buffer.size() bytes from that
   * register.
   *
   * @param[in] target_address 7-bit or 10-bit I2C target address.
   * @param[in] is_10bit_address Whether the target address is 10-bit (true) or
   * 7-bit (false).
   * @param[in]  register_address    Register address to read from.
   * @param[in]  register_size_bytes Width of the register address field in
   * bytes (1–4).
   * @param[out] buffer              Destination span that receives the register
   * data.
   * @param[in]  timeout_ms          Maximum time to wait for the transaction,
   * in milliseconds.
   * @retval ErrorCode::kOk      The register was read successfully.
   * @retval ErrorCode::kTimeout The transaction did not complete within @p
   * timeout_ms.
   * @retval ErrorCode::kError   A bus error or NACK was detected.
   */
  virtual ErrorCode ReadRegister(uint16_t target_address, bool is_10bit_address,
                                 uint32_t register_address,
                                 uint8_t register_size_bytes,
                                 std::span<uint8_t> buffer,
                                 uint32_t timeout_ms) = 0;

  /**
   * @brief Writes bytes to a specific register address of the target device.
   *
   * Transmits the register address (MSB first, @p register_size_bytes wide)
   * followed by all bytes in @p buffer in a single I2C write transaction.
   *
   * @param[in] target_address 7-bit or 10-bit I2C target address.
   * @param[in] is_10bit_address Whether the target address is 10-bit (true) or
   * 7-bit (false).
   * @param[in] register_address    Register address to write to.
   * @param[in] register_size_bytes Width of the register address field in bytes
   * (1–4).
   * @param[in] buffer              Source span containing the data to write.
   * @param[in] timeout_ms          Maximum time to wait for the transaction, in
   * milliseconds.
   * @retval ErrorCode::kOk      The register was written successfully.
   * @retval ErrorCode::kTimeout The transaction did not complete within @p
   * timeout_ms.
   * @retval ErrorCode::kError   A bus error or NACK was detected.
   */
  virtual ErrorCode WriteRegister(uint16_t target_address,
                                  bool is_10bit_address,
                                  uint32_t register_address,
                                  uint8_t register_size_bytes,
                                  std::span<const uint8_t> buffer,
                                  uint32_t timeout_ms) = 0;

  /**
   * @brief Detects the presence of a target at the specified I2C address.
   *
   * This method can be used to check if a device is present at the given
   * address. It will perform only one try. Repeated attempts must be handled by
   * the user. The detection method is not defined, but it typically involves
   * sending the target address and checking for an acknowledgment.
   *
   * @param target_address 7-bit or 10-bit I2C target address.
   * @param is_10bit_address Whether the target address is 10-bit (true) or
   * 7-bit (false).
   * @param timeout_ms Maximum time to wait for the detection, in milliseconds.
   * @retval ErrorCode::kOk if the device is detected.
   * @retval ErrorCode::kTimeout if the detection did not complete within @p
   * timeout_ms.
   * @retval ErrorCode::kError if a bus error or NACK was detected.
   */
  virtual ErrorCode Detect(uint16_t target_address, bool is_10bit_address,
                           uint32_t timeout_ms) = 0;
};

}  // namespace sfw::hal_interface

#endif  // HAL_INTERFACE_I2C_CONTROLLER_HPP