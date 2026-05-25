// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_LINUX_SPI_SPI_CONTROLLER_HPP
#define HAL_LINUX_SPI_SPI_CONTROLLER_HPP

#include <array>
#include <cstdint>
#include <span>
#include <string>

#include "hal_interface/error_code.hpp"
#include "hal_interface/spi_controller.hpp"

namespace sfw::hal_linux {

/**
 * @brief Linux SPI controller using spidev.
 *
 * This class wraps a Linux spidev device file and implements all SPI transfer
 * methods from hal_interface::SpiController.
 * All configurable Linux SPI parameters are injected through the constructor.
 * The linux api for spi does not support timeouts, so the timeout parameters on
 * the interface methods are currently unused.
 *
 * Include path: `#include "hal_linux/spi/spi_controller.hpp"`
 *
 * Typical usage:
 * 1. Construct SpiController with the desired configuration.
 * 2. Call Initialize() once to open and configure the spidev device.
 * 3. Call Read(), Write(), WriteThenRead(), Transfer(), ReadRegister(), or
 *    WriteRegister() with a configured chip-select digital output.
 * 4. Call Deinitialize() when the SPI peripheral is no longer needed.
 */
class SpiController final : public hal_interface::SpiController {
 public:
  /**
   * @brief Constructs a Linux SPI controller.
   *
   * @param[in] device Path to the spidev device.
   * @param[in] mode SPI mode (0-3).
   * @param[in] clock_hz SPI clock frequency in Hz.
   * @param[in] bits_per_word Number of bits per word (default: 8).
   */
  explicit SpiController(std::string device, uint32_t mode, uint32_t clock_hz,
                         uint8_t bits_per_word = 8);  // NOLINT

  /**
   * @brief Destructor
   */
  ~SpiController() override;

  SpiController(const SpiController&) = delete;
  SpiController& operator=(const SpiController&) = delete;
  SpiController(SpiController&&) = delete;
  SpiController& operator=(SpiController&&) = delete;

  /**
   * @brief Opens and configures the Linux spidev device.
   *
   * @retval ErrorCode::kOk Device opened and configured successfully.
   * @retval ErrorCode::kError open() or one of the configuration ioctls
   * failed.
   */
  hal_interface::ErrorCode Initialize() override;

  /**
   * @brief Closes the Linux spidev device.
   *
   * @retval ErrorCode::kOk Device closed successfully.
   * @retval ErrorCode::kError close() failed.
   */
  hal_interface::ErrorCode Deinitialize() override;

  /**
   * @brief Returns whether the Linux spidev device is initialized.
   *
   * @retval true The SPI device is open and configured.
   * @retval false The SPI device is not initialized.
   */
  bool IsInitialized() override;

  /**
   * @brief Reads bytes from SPI while sending dummy bytes.
   *
   * Timeout is currently not implemented because the Linux SPI API does not
   * support it.
   *
   * @param[in,out] cs_pin Chip-select output used for this transaction.
   * @param[in] active_state Active level used to assert chip-select.
   * @param[out] buffer Destination span receiving the read bytes.
   * @param[in] timeout_ms Maximum transaction time in milliseconds.
   * @retval ErrorCode::kOk Read completed successfully.
   * @retval ErrorCode::kTimeout Transaction time exceeded @p timeout_ms.
   * @retval ErrorCode::kError Invalid state or Linux SPI I/O failure.
   */
  hal_interface::ErrorCode Read(hal_interface::DigitalOutput& cs_pin,
                                bool active_state, std::span<uint8_t> buffer,
                                uint32_t timeout_ms) override;

  /**
   * @brief Writes bytes to SPI and discards incoming bytes.
   *
   * Timeout is currently not implemented because the Linux SPI API does not
   * support it.
   *
   * @param[in,out] cs_pin Chip-select output used for this transaction.
   * @param[in] active_state Active level used to assert chip-select.
   * @param[in] buffer Source span containing bytes to transmit.
   * @param[in] timeout_ms Maximum transaction time in milliseconds.
   * @retval ErrorCode::kOk Write completed successfully.
   * @retval ErrorCode::kTimeout Transaction time exceeded @p timeout_ms.
   * @retval ErrorCode::kError Invalid state or Linux SPI I/O failure.
   */
  hal_interface::ErrorCode Write(hal_interface::DigitalOutput& cs_pin,
                                 bool active_state,
                                 std::span<const uint8_t> buffer,
                                 uint32_t timeout_ms) override;

  /**
   * @brief Writes then reads in one CS assertion window.
   *
   * Timeout is currently not implemented because the Linux SPI API does not
   * support it.
   *
   * @param[in,out] cs_pin Chip-select output used for this transaction.
   * @param[in] active_state Active level used to assert chip-select.
   * @param[in] write_buffer Source span transmitted first.
   * @param[out] read_buffer Destination span receiving bytes after write.
   * @param[in] timeout_ms Maximum transaction time in milliseconds.
   * @retval ErrorCode::kOk Sequence completed successfully.
   * @retval ErrorCode::kTimeout Transaction time exceeded @p timeout_ms.
   * @retval ErrorCode::kError Invalid state or Linux SPI I/O failure.
   */
  hal_interface::ErrorCode WriteThenRead(hal_interface::DigitalOutput& cs_pin,
                                         bool active_state,
                                         std::span<const uint8_t> write_buffer,
                                         std::span<uint8_t> read_buffer,
                                         uint32_t timeout_ms) override;

  /**
   * @brief Performs a full-duplex SPI transfer.
   *
   * Timeout is currently not implemented because the Linux SPI API does not
   * support it.
   *
   * @param[in,out] cs_pin Chip-select output used for this transaction.
   * @param[in] active_state Active level used to assert chip-select.
   * @param[in] write_buffer Source span containing bytes to transmit.
   * @param[out] read_buffer Destination span receiving bytes.
   * @param[in] timeout_ms Maximum transaction time in milliseconds.
   * @retval ErrorCode::kOk Transfer completed successfully.
   * @retval ErrorCode::kTimeout Transaction time exceeded @p timeout_ms.
   * @retval ErrorCode::kError Invalid state, size mismatch, or I/O failure.
   */
  hal_interface::ErrorCode Transfer(hal_interface::DigitalOutput& cs_pin,
                                    bool active_state,
                                    std::span<const uint8_t> write_buffer,
                                    std::span<uint8_t> read_buffer,
                                    uint32_t timeout_ms) override;

  /**
   * @brief Reads bytes from a register-addressed SPI target.
   *
   * Timeout is currently not implemented because the Linux SPI API does not
   * support it.
   *
   * @param[in,out] cs_pin Chip-select output used for this transaction.
   * @param[in] active_state Active level used to assert chip-select.
   * @param[in] register_address Register address to read.
   * @param[in] register_size_bytes Register address width in bytes (1 to 4).
   * @param[out] buffer Destination span receiving register data.
   * @param[in] timeout_ms Maximum transaction time in milliseconds.
   * @retval ErrorCode::kOk Register read completed successfully.
   * @retval ErrorCode::kTimeout Transaction time exceeded @p timeout_ms.
   * @retval ErrorCode::kError Invalid parameters, state, or I/O failure.
   */
  hal_interface::ErrorCode ReadRegister(hal_interface::DigitalOutput& cs_pin,
                                        bool active_state,
                                        uint32_t register_address,
                                        uint8_t register_size_bytes,
                                        std::span<uint8_t> buffer,
                                        uint32_t timeout_ms) override;

  /**
   * @brief Writes bytes to a register-addressed SPI target.
   *
   * Timeout is currently not implemented because the Linux SPI API does not
   * support it.
   *
   * @param[in,out] cs_pin Chip-select output used for this transaction.
   * @param[in] active_state Active level used to assert chip-select.
   * @param[in] register_address Register address to write.
   * @param[in] register_size_bytes Register address width in bytes (1 to 4).
   * @param[in] buffer Source span containing register payload bytes.
   * @param[in] timeout_ms Maximum transaction time in milliseconds.
   * @retval ErrorCode::kOk Register write completed successfully.
   * @retval ErrorCode::kTimeout Transaction time exceeded @p timeout_ms.
   * @retval ErrorCode::kError Invalid parameters, state, or I/O failure.
   */
  hal_interface::ErrorCode WriteRegister(hal_interface::DigitalOutput& cs_pin,
                                         bool active_state,
                                         uint32_t register_address,
                                         uint8_t register_size_bytes,
                                         std::span<const uint8_t> buffer,
                                         uint32_t timeout_ms) override;

 private:
  [[nodiscard]] hal_interface::ErrorCode CloseBus();
  [[nodiscard]] hal_interface::ErrorCode ApplyConfiguration() const;
  [[nodiscard]] static bool IsRegSizeValid(uint8_t register_size_bytes);
  [[nodiscard]] static std::array<
      uint8_t, hal_interface::SpiController::kMaxRegAddressSize>
  EncodeRegAddress(uint32_t register_address, uint8_t register_size_bytes);

  int fd_{-1};
  bool initialized_{false};
  std::string device_;
  uint32_t mode_;
  uint8_t bits_per_word_;
  uint32_t clock_hz_;
};

}  // namespace sfw::hal_linux

#endif  // HAL_LINUX_SPI_SPI_CONTROLLER_HPP
