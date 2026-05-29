// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_LINUX_I2C_I2C_CONTROLLER_HPP
#define HAL_LINUX_I2C_I2C_CONTROLLER_HPP

#include <cstdint>
#include <span>
#include <string>

#include "hal_interface/error_code.hpp"
#include "hal_interface/i2c_controller.hpp"

namespace sfw::hal_linux {

/**
 * @brief Linux I2C bus controller for communicating with multiple targets.
 *
 * Wraps a Linux I2C bus device (for example, /dev/i2c-0) and provides
 * synchronous transfer methods for raw payloads and register-addressed
 * transfers.
 *
 * On Linux, the actual bus bitrate is typically configured by platform
 * firmware, device tree, or kernel driver. Thus, it cannot be configured at
 * the user space level.
 *
 * Include path: `#include "hal_linux/i2c/i2c_controller.hpp`
 *
 * Typical usage:
 * 1. Construct I2cController with a Linux I2C device path (for example,
 * /dev/i2c-0).
 * 2. Call Initialize() once to configure the I2C hardware.
 * 3. Call Read(), Write(), or WriteThenRead() to communicate with a target;
 * those methods require a valid 7-bit or 10-bit target address.
 * 4. Call ReadRegister() or WriteRegister() for register-based access.
 * 5. Call Deinitialize() when the bus is no longer required.
 */
class I2cController final : public hal_interface::I2cController {
 public:
  /**
   * @brief Constructs an I2C bus controller instance.
   *
   * @param[in] device Linux I2C device path (for example, "/dev/i2c-0").
   */
  explicit I2cController(std::string device);

  /**
   * @brief Destructor
   */
  ~I2cController() override;

  I2cController(const I2cController&) = delete;
  I2cController& operator=(const I2cController&) = delete;
  I2cController(I2cController&&) = delete;
  I2cController& operator=(I2cController&&) = delete;

  /**
   * @brief Initializes the Linux I2C bus device.
   *
   * Opens the configured bus device.
   *
   * @retval ErrorCode::kOk Bus opened successfully.
   * @retval ErrorCode::kError Invalid address or open/select failure.
   */
  hal_interface::ErrorCode Initialize() override;

  /**
   * @brief Deinitializes the Linux I2C bus device.
   *
   * @retval ErrorCode::kOk Bus closed successfully.
   * @retval ErrorCode::kError close() failed.
   */
  hal_interface::ErrorCode Deinitialize() override;

  /**
   * @brief Returns whether the bus has been initialized.
   *
   * @retval true The bus device is open and ready.
   * @retval false The bus device is not initialized.
   */
  bool IsInitialized() override;

  /**
   * @brief Reads bytes from an I2C target.
   *
   * @param[in] target_address 7-bit or 10-bit I2C target address.
   * @param[in] is_10bit_address Whether the target address is 10-bit (true) or
   * 7-bit (false).
   * @param[out] buffer Destination span for received bytes.
   * @param[in] timeout_ms Maximum transfer timeout in milliseconds.
   * @retval ErrorCode::kOk All bytes read successfully.
   * @retval ErrorCode::kTimeout The transfer completed with fewer bytes.
   * @retval ErrorCode::kError Invalid state, invalid address, or I/O failure.
   */
  hal_interface::ErrorCode Read(uint16_t target_address, bool is_10bit_address,
                                std::span<uint8_t> buffer,
                                uint32_t timeout_ms) override;

  /**
   * @brief Writes bytes to an I2C target.
   *
   * @param[in] target_address 7-bit or 10-bit I2C target address.
   * @param[in] is_10bit_address Whether the target address is 10-bit (true) or
   * 7-bit (false).
   * @param[in] buffer Source span containing bytes to transmit.
   * @param[in] timeout_ms Maximum transfer timeout in milliseconds.
   * @retval ErrorCode::kOk All bytes written successfully.
   * @retval ErrorCode::kTimeout The transfer completed with fewer bytes.
   * @retval ErrorCode::kError Invalid state, invalid address, or I/O failure.
   */
  hal_interface::ErrorCode Write(uint16_t target_address, bool is_10bit_address,
                                 std::span<const uint8_t> buffer,
                                 uint32_t timeout_ms) override;

  /**
   * @brief Writes then reads without releasing the bus.
   *
   * Uses the Linux I2C_RDWR ioctl to execute a combined transaction with a
   * repeated START between write and read phases.
   *
   * Implementation details: for small reads this method uses a single
   * I2C_RDWR combined transaction. For larger reads, it falls back to a
   * Write() call followed by a Read() call to avoid adapter-specific faults
   * observed with larger combined-read transactions.
   *
   * @param[in] target_address 7-bit or 10-bit I2C target address.
   * @param[in] is_10bit_address Whether the target address is 10-bit (true) or
   * 7-bit (false).
   * @param[in] write_buffer Source span containing bytes to transmit.
   * @param[out] read_buffer Destination span for received bytes.
   * @param[in] timeout_ms Maximum transfer timeout in milliseconds.
   * @retval ErrorCode::kOk Transaction completed successfully.
   * @retval ErrorCode::kError Invalid state, invalid address, or I/O failure.
   */
  hal_interface::ErrorCode WriteThenRead(uint16_t target_address,
                                         bool is_10bit_address,
                                         std::span<const uint8_t> write_buffer,
                                         std::span<uint8_t> read_buffer,
                                         uint32_t timeout_ms) override;

  /**
   * @brief Reads bytes from a register of an I2C target.
   *
   * Register address is serialized as MSB-first with width controlled by
   * @p register_size_bytes. The register address is saved in a std::array of
   * size 4 bytes, independently of @p register_size_bytes. Then,
   * WriteThenRead() is called with the encoded register address as the write
   * buffer and @p buffer as the read destination.
   *
   * @param[in] target_address 7-bit or 10-bit I2C target address.
   * @param[in] is_10bit_address Whether the target address is 10-bit (true) or
   * 7-bit (false).
   * @param[in] register_address Register address to read from.
   * @param[in] register_size_bytes Register address width in bytes (1 to 4).
   * @param[out] buffer Destination span for received bytes.
   * @param[in] timeout_ms Maximum transfer timeout in milliseconds.
   * @retval ErrorCode::kOk Register read completed successfully.
   * @retval ErrorCode::kError Invalid parameters, invalid state, or I/O
   * failure.
   */
  hal_interface::ErrorCode ReadRegister(uint16_t target_address,
                                        bool is_10bit_address,
                                        uint32_t register_address,
                                        uint8_t register_size_bytes,
                                        std::span<uint8_t> buffer,
                                        uint32_t timeout_ms) override;

  /**
   * @brief Writes bytes to a register of an I2C target.
   *
   * Register address is serialized as MSB-first with width controlled by
   * @p register_size_bytes. Since there is no standard for register writes on
   * Linux, this method concatenates the register address and payload into a
   * single buffer and uses Write() to perform the write operation. It
   * means this method will copy all the content of @p buffer to a separate
   * buffer. This buffer is created as an std::vector, which means dynamic
   * memory allocation may occur.
   *
   * @param[in] target_address 7-bit or 10-bit I2C target address.
   * @param[in] is_10bit_address Whether the target address is 10-bit (true) or
   * 7-bit (false).
   * @param[in] register_address Register address to write to.
   * @param[in] register_size_bytes Register address width in bytes (1 to 4).
   * @param[in] buffer Source span containing register payload bytes.
   * @param[in] timeout_ms Maximum transfer timeout in milliseconds.
   * @retval ErrorCode::kOk Register write completed successfully.
   * @retval ErrorCode::kError Invalid parameters, invalid state, or I/O
   * failure.
   */
  hal_interface::ErrorCode WriteRegister(uint16_t target_address,
                                         bool is_10bit_address,
                                         uint32_t register_address,
                                         uint8_t register_size_bytes,
                                         std::span<const uint8_t> buffer,
                                         uint32_t timeout_ms) override;
  /**
   * @brief Detects the presence of a target at the specified I2C address.
   *
   * This implementation performs an smbus read byte operation, which is a
   * common way to probe for device presence on Linux. Note that some targets
   * may not support this operation or may have side effects when accessed, so
   * use with caution.
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
  hal_interface::ErrorCode Detect(uint16_t target_address,
                                  bool is_10bit_address,
                                  uint32_t timeout_ms) override;

 private:
  [[nodiscard]] hal_interface::ErrorCode ConfigureTransactionTimeout(
      uint32_t timeout_ms) const;
  [[nodiscard]] hal_interface::ErrorCode SelectTarget(
      uint16_t target_address, bool is_10bit_address) const;

  std::string device_;
  int fd_{-1};
  bool initialized_{false};
};

}  // namespace sfw::hal_linux

#endif  // HAL_LINUX_I2C_I2C_CONTROLLER_HPP
