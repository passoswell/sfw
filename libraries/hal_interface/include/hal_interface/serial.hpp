// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_INTERFACE_SERIAL_HPP
#define HAL_INTERFACE_SERIAL_HPP

#include <cstdint>
#include <span>

#include "hal_interface/error_code.hpp"

namespace sfw::hal_interface {

/**
 * @brief Abstract interface for serial-like peripherals.
 *
 * Provides byte-stream read and write operations. This is intended to cover
 * UART peripherals, USB CDC-ACM, and potentially other character-oriented
 * interfaces that don't fit the traditional UART model.
 * All the methods on this interface are synchronous / blocking. This means
 * they only return if the operation is finished or if an error occurred.
 * However, implementations may use DMA, interrupts, or polled transfers
 * internally.
 *
 * Typical usage:
 * 1. Call Initialize() once to configure the UART (baud rate, data bits,
 * parity, etc.).
 * 2. Call Write() to transmit data to the serial port.
 * 3. Call Read() to receive incoming data, or BytesAvailable() to check for
 * pending data.
 * 4. Call ClearReadBuffer() to flush stale data if needed.
 * 5. Call Deinitialize() when communication is no longer needed.
 */
class Serial {
 public:
  Serial() = default;
  Serial(const Serial&) = default;
  Serial& operator=(const Serial&) = default;
  Serial(Serial&&) = default;
  Serial& operator=(Serial&&) = default;

  /**
   * @brief Destructor
   */
  virtual ~Serial() = default;

  /**
   * @brief Parity mode for the UART frame.
   */
  enum class Parity : uint8_t {
    kNone = 0,  ///< No parity bit.
    kOdd,       ///< Odd parity.
    kEven,      ///< Even parity.
  };

  /**
   * @brief Number of stop bits per UART frame.
   */
  enum class StopBits : uint8_t {
    k1 = 0,  ///< One stop bit.
    k2,      ///< Two stop bits.
  };

  /**
   * @brief Number of data bits per UART frame.
   */
  enum class DataBits : uint8_t {
    k5 = 5,  ///< 5 data bits.
    k6 = 6,  ///< 6 data bits.
    k7 = 7,  ///< 7 data bits.
    k8 = 8,  ///< 8 data bits.
  };

  /**
   * @brief Usage of hardware flow control (e.g. RTS/CTS).
   */
  enum class FlowControl : uint8_t {
    kNone = 0,  ///< No flow control.
    kRtsCts,    ///< RTS/CTS hardware flow control.
  };

  /**
   * @brief Initializes the serial peripheral.
   *
   * Configures the UART hardware (baud rate, data bits, parity, stop bits,
   * and GPIO pins). These parameters may be set through the constructor of the
   * implementation.
   * Must be called before any other method.
   *
   * @retval ErrorCode::kOk    Initialization succeeded.
   * @retval ErrorCode::kError Initialization failed due to a hardware fault.
   */
  virtual ErrorCode Initialize() = 0;

  /**
   * @brief Deinitializes the serial peripheral.
   *
   * Releases hardware resources allocated by Initialize(). After this call,
   * only Initialize() and IsInitialized() are valid entry points.
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
   * @brief Reads up to @p buffer.size() bytes from the receive buffer.
   *
   * Blocks until all desired bytes are read or @p timeout_ms elapses, whichever
   * comes first. The actual number of bytes copied into @p buffer is written to
   * @p bytes_read and may be less than @p buffer.size() if fewer bytes
   * arrived before the timeout. Call this method repeatedly to retrieve all
   * pending data.
   * The implementations must be buffered. A buffered read means the peripheral
   * (or a task) is running in the background saving the data received into a
   * buffer internal to the implementation. In this case, the Read() operation
   * will copy this data from the internal buffer to the user buffer and wait
   * for more until timeout, if needed. If no data is received before timeout,
   * the method will return with @p bytes_read set to 0.
   *
   * @param[out] buffer     Destination span that receives the incoming bytes.
   * @param[out] bytes_read Number of bytes actually placed into @p buffer.
   * @param[in]  timeout_ms Maximum time to wait for at least one byte, in
   * milliseconds.
   * @retval ErrorCode::kOk      At least one byte was read successfully.
   * @retval ErrorCode::kTimeout No bytes arrived within @p timeout_ms.
   */
  virtual ErrorCode Read(std::span<uint8_t> buffer, uint16_t& bytes_read,
                         uint32_t timeout_ms) = 0;

  /**
   * @brief Reads up to @p buffer.size() bytes from the receive buffer.
   *
   * Blocks until all desired bytes are read or @p timeout_ms elapses, whichever
   * comes first. The actual number of bytes copied into @p buffer is written to
   * @p bytes_read and may be less than @p buffer.size() if fewer bytes
   * arrived before the timeout. Call this method repeatedly to retrieve all
   * pending data.
   * The implementations must be buffered. A buffered read means the peripheral
   * (or a task) is running in the background saving the data received into a
   * buffer internal to the implementation. In this case, the Read() operation
   * will copy this data from the internal buffer to the user buffer and wait
   * for more until timeout, if needed. If no data is received before timeout,
   * the method will return with @p bytes_read set to 0.
   *
   * @param[out] buffer     Destination span that receives the incoming bytes.
   * @param[out] bytes_read Number of bytes actually placed into @p buffer.
   * @param[in]  timeout_ms Maximum time to wait for at least one byte, in
   * milliseconds.
   * @retval ErrorCode::kOk      At least one byte was read successfully.
   * @retval ErrorCode::kTimeout No bytes arrived within @p timeout_ms.
   */
  virtual ErrorCode Read(std::span<char> buffer, uint16_t& bytes_read,
                         uint32_t timeout_ms) = 0;

  /**
   * @brief Transmits all bytes in @p buffer over the serial line.
   *
   * The implementation may accept a constructor parameter defining if a call to
   * Write() should block until the hardware has completed the transmission of
   * all bytes, or if it should return as soon as the bytes have been accepted
   * into an internal buffer.
   * If this parameter is not configurable, the implementation must document the
   * implemented behavior.
   * If the implementation uses an internal buffer, it must have the means to
   * complete the transmission even after Write() returns. In the case the
   * internal buffer is not large enough to hold all bytes passed to Write(),
   * the implementation must implement a logic to ensure all bytes are
   * eventually copied to the internal buffer while respecting the timeout limit
   * imposed by @p timeout_ms. This means the data may not be fully copied /
   * transmitted by the time  Write() returns, in which case ErrorCode::kTimeout
   * will be returned.
   *
   * @param[in] buffer     Source span containing the bytes to transmit.
   * @param[in] timeout_ms Maximum time to wait for the transmission to
   * complete, in milliseconds.
   * @retval ErrorCode::kOk      All bytes were copied to an internal buffer /
   * transmitted successfully.
   * @retval ErrorCode::kTimeout The copy / transmission did not complete within
   * @p timeout_ms.
   */
  virtual ErrorCode Write(std::span<const uint8_t> buffer,
                          uint32_t timeout_ms) = 0;

  /**
   * @brief Transmits all characters in @p buffer over the serial line.
   *
   * The implementation may accept a constructor parameter defining if a call to
   * Write() should block until the hardware has completed the transmission of
   * all bytes, or if it should return as soon as the bytes have been accepted
   * into an internal buffer.
   * If this parameter is not configurable, the implementation must document the
   * implemented behavior.
   * If the implementation uses an internal buffer, it must have the means to
   * complete the transmission even after Write() returns. In the case the
   * internal buffer is not large enough to hold all bytes passed to Write(),
   * the implementation must implement a logic to ensure all bytes are
   * eventually copied to the internal buffer while respecting the timeout limit
   * imposed by @p timeout_ms. This means the data may not be fully copied /
   * transmitted by the time  Write() returns, in which case ErrorCode::kTimeout
   * will be returned.
   *
   * @param[in] buffer     Source span containing the bytes to transmit.
   * @param[in] timeout_ms Maximum time to wait for the transmission to
   * complete, in milliseconds.
   * @retval ErrorCode::kOk      All bytes were copied to an internal buffer /
   * transmitted successfully.
   * @retval ErrorCode::kTimeout The copy / transmission did not complete within
   * @p timeout_ms.
   */
  virtual ErrorCode Write(std::span<const char> buffer,
                          uint32_t timeout_ms) = 0;

  /**
   * @brief Returns the number of bytes currently waiting in the receive buffer.
   *
   * May be used to avoid blocking in Read() when there are not enough bytes
   * available in the internal buffer.
   *
   * @retval Number of bytes available to read without blocking.
   */
  virtual uint16_t BytesAvailable() = 0;

  /**
   * @brief Discards all bytes currently in the receive buffer.
   *
   * Useful for flushing stale data from the internal buffer, if existent,
   * before starting a new protocol exchange. In case the implementation does
   * not use an internal buffer, this method must always return ErrorCode::kOk
   * without doing anything.
   *
   * @retval ErrorCode::kOk The receive buffer was cleared successfully.
   */
  virtual ErrorCode ClearReadBuffer() = 0;
};

}  // namespace sfw::hal_interface

#endif  // HAL_INTERFACE_SERIAL_HPP