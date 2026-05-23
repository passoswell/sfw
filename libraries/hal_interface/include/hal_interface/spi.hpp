// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_INTERFACE_SPI_HPP
#define HAL_INTERFACE_SPI_HPP

#include <cstdint>
#include <span>

#include "hal_interface/error_code.hpp"

namespace sfw::hal_interface {

/**
 * @brief Abstract interface for an SPI controller.
 *
 * Provides half-duplex read, write, and write-then-read operations as well as
 * full-duplex transfer for accessing SPI targets.
 * Register-addressed helpers are available for targets such as memory ICs.
 * An SPI controller is the peripheral/device responsible for driving the
 * chip select signals in the bus, while the targets are the devices selected.
 * The chip-select signal management and clock/mode configuration are assumed
 * to be handled by the concrete implementation. These parameters may be set
 * through the constructor of the implementation class.
 * All the methods on this interface are synchronous / blocking. This means
 * they only return if the operation is finished or if an error occurred.
 *
 * Typical usage:
 * 1. Call Initialize() once to configure the SPI hardware. The parameters,
 *    including clock polarity/phase, bit order, GPIO pins, etc., are handled by
 *    the concrete implementation and possibly set through the constructor.
 * 2. Call Read(), Write(), or WriteThenRead() for half-duplex operations.
 * 3. Call Transfer() for full-duplex simultaneous transmit and receive.
 * 4. Call ReadRegister() or WriteRegister() for register-based operations with
 *    SPI targets.
 * 5. Call Deinitialize() when the peripheral is no longer needed.
 */
class SpiController {
 public:
  SpiController() = default;
  SpiController(const SpiController&) = default;
  SpiController& operator=(const SpiController&) = default;
  SpiController(SpiController&&) = default;
  SpiController& operator=(SpiController&&) = default;

  /**
   * @brief Destructor
   */
  virtual ~SpiController() = default;

  /**
   * @brief Initializes the SPI controller peripheral.
   *
   * Configures the SPI hardware (clock polarity/phase, bit order, GPIO pins,
   * etc.). These parameters, if needed, must be passed through the constructor
   * of the concrete implementation. Must be called before any transfer method.
   *
   * @retval ErrorCode::kOk    Initialization succeeded.
   * @retval ErrorCode::kError Initialization failed due to a hardware fault.
   */
  virtual ErrorCode Initialize() = 0;

  /**
   * @brief Deinitializes the SPI controller peripheral.
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
   * @brief Reads bytes from the SPI bus into @p buffer, transmitting
   * dummy bytes.
   *
   * Asserts the chip-select, clocks out @p buffer.size() dummy bytes (typically
   * 0x00 or 0xFF), captures the simultaneously received bytes into @p buffer,
   * and de-asserts the chip-select.
   *
   * @param[out] buffer     Destination span that receives the read bytes.
   * @param[in]  timeout_ms Maximum time to wait for the transfer, in
   * milliseconds.
   * @retval ErrorCode::kOk      All bytes were read successfully.
   * @retval ErrorCode::kTimeout The transfer did not complete within
   * @p timeout_ms.
   */
  virtual ErrorCode Read(std::span<uint8_t> buffer, uint32_t timeout_ms) = 0;

  /**
   * @brief Writes bytes from @p buffer to the SPI bus, discarding
   * received data.
   *
   * Asserts the chip-select, clocks out all bytes in @p buffer, discards the
   * simultaneously received bytes, and de-asserts the chip-select.
   *
   * @param[in] buffer     Source span containing the bytes to transmit.
   * @param[in] timeout_ms Maximum time to wait for the transfer, in
   * milliseconds.
   * @retval ErrorCode::kOk      All bytes were written successfully.
   * @retval ErrorCode::kTimeout The transfer did not complete within
   * @p timeout_ms.
   */
  virtual ErrorCode Write(std::span<const uint8_t> buffer,
                          uint32_t timeout_ms) = 0;

  /**
   * @brief Writes @p write_buffer then reads @p read_buffer.size() bytes
   * without releasing CS in the meantime.
   *
   * The chip-select is held asserted for the entire sequence. This pattern
   * is commonly used to send a command or address and then receive the
   * device's response.
   *
   * @param[in]  write_buffer Source span containing the bytes to transmit.
   * @param[out] read_buffer  Destination span that receives the bytes read
   * after the write.
   * @param[in]  timeout_ms   Maximum time to wait for the entire sequence, in
   * milliseconds.
   * @retval ErrorCode::kOk      The sequence completed successfully.
   * @retval ErrorCode::kTimeout The transfer did not complete within
   * @p timeout_ms.
   */
  virtual ErrorCode WriteThenRead(std::span<const uint8_t> write_buffer,
                                  std::span<uint8_t> read_buffer,
                                  uint32_t timeout_ms) = 0;

  /**
   * @brief Performs a full-duplex simultaneous write and read.
   *
   * Both @p write_buffer and @p read_buffer must have the same size. For each
   * clock cycle, one byte from @p write_buffer is transmitted while the
   * received byte is stored in the corresponding position of @p read_buffer.
   *
   * @param[in]  write_buffer Source span containing the bytes to transmit.
   * @param[out] read_buffer  Destination span that receives the simultaneously
   * captured bytes.
   * @param[in]  timeout_ms   Maximum time to wait for the transfer, in
   * milliseconds.
   * @retval ErrorCode::kOk      The full-duplex transfer completed
   * successfully.
   * @retval ErrorCode::kTimeout The transfer did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode Transfer(std::span<const uint8_t> write_buffer,
                             std::span<uint8_t> read_buffer,
                             uint32_t timeout_ms) = 0;

  /**
   * @brief Reads bytes from a specific register address of the target device.
   *
   * Transmits the register address (@p register_size_bytes wide),
   * then reads @p buffer.size() bytes in a single chip-select assertion.
   *
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
   */
  virtual ErrorCode ReadRegister(uint32_t register_address,
                                 uint8_t register_size_bytes,
                                 std::span<uint8_t> buffer,
                                 uint32_t timeout_ms) = 0;

  /**
   * @brief Writes bytes to a specific register address of the target device.
   *
   * Transmits the register address (@p register_size_bytes wide)
   * followed by all bytes in @p buffer in a single chip-select assertion.
   *
   * @param[in] register_address    Register address to write to.
   * @param[in] register_size_bytes Width of the register address field in bytes
   * (1–4).
   * @param[in] buffer              Source span containing the data to write.
   * @param[in] timeout_ms          Maximum time to wait for the transaction, in
   * milliseconds.
   * @retval ErrorCode::kOk      The register was written successfully.
   * @retval ErrorCode::kTimeout The transaction did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode WriteRegister(uint32_t register_address,
                                  uint8_t register_size_bytes,
                                  std::span<const uint8_t> buffer,
                                  uint32_t timeout_ms) = 0;
};

}  // namespace sfw::hal_interface

#endif  // HAL_INTERFACE_SPI_HPP