// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_LINUX_UART_UART_SERIAL_HPP
#define HAL_LINUX_UART_UART_SERIAL_HPP

#include <cstdint>
#include <span>
#include <string>

#include "hal_interface/serial.hpp"

namespace sfw::hal_linux {

/**
 * @brief Linux UART implementation of the hal_interface::Serial interface.
 *
 * Wraps a Linux tty device (e.g. /dev/ttyS0, /dev/ttyUSB0) using POSIX
 * termios to configure baud rate, data bits, parity, and stop bits.
 * All read and write operations are synchronous and use poll() for
 * timeout enforcement. Incoming bytes that arrive between Read() calls
 * are buffered by the kernel; BytesAvailable() queries the kernel buffer
 * via ioctl(FIONREAD).
 *
 * Include path: `#include "hal_linux/uart/uart_serial.hpp`
 *
 * Typical usage:
 * 1. Construct a UartSerial with the device path and desired settings,
 *    e.g. UartSerial uart("/dev/ttyUSB0", 115200).
 * 2. Call Initialize() to open the device and apply termios settings.
 * 3. Call Write() to transmit data, passing a byte span and timeout.
 * 4. Call Read() to receive data into a byte span with a timeout;
 *    check bytes_read for the actual number of bytes received.
 * 5. Optionally call BytesAvailable() to check pending bytes before
 *    blocking on Read().
 * 6. Optionally call ClearReadBuffer() to flush stale kernel data.
 * 7. Call Deinitialize() when communication is no longer needed to
 *    close the file descriptor.
 */
class UartSerial final : public hal_interface::Serial {
 public:
  /**
   * @brief Constructs a UartSerial instance with the given UART settings.
   *
   * The device is not opened until Initialize() is called.
   *
   * @param[in] device    Path to the tty device (e.g. "/dev/ttyUSB0").
   * @param[in] baud_rate Desired baud rate (e.g. 9600, 115200).
   * @param[in] wait_end_of_transmission If true, Write() will block until the
   * hardware has completed transmission of all bytes through the wire. If
   * false, Write() will return as soon as the bytes have been accepted into the
   * kernel buffer. Defaults to true.
   * @param[in] flow_control Flow control setting (default: none).
   * @param[in] data_bits Number of data bits per frame (default: 8).
   * @param[in] parity    Parity mode (default: none).
   * @param[in] stop_bits Number of stop bits per frame (default: 1).
   */
  explicit UartSerial(std::string device, uint32_t baud_rate,
                      bool wait_end_of_transmission = true,
                      hal_interface::Serial::FlowControl flow_control =
                          hal_interface::Serial::FlowControl::kNone,
                      hal_interface::Serial::DataBits data_bits =
                          hal_interface::Serial::DataBits::k8,
                      hal_interface::Serial::Parity parity =
                          hal_interface::Serial::Parity::kNone,
                      hal_interface::Serial::StopBits stop_bits =
                          hal_interface::Serial::StopBits::k1);

  /**
   * @brief Destructor
   *
   * Closes the file descriptor if it is still open.
   */
  ~UartSerial() override;

  UartSerial(const UartSerial&) = delete;
  UartSerial& operator=(const UartSerial&) = delete;
  UartSerial(UartSerial&&) = delete;
  UartSerial& operator=(UartSerial&&) = delete;

  /**
   * @brief Opens the tty device and configures it via termios.
   *
   * Opens @p device_ with O_RDWR | O_NOCTTY | O_NONBLOCK, then applies
   * termios settings derived from the constructor parameters. The file
   * descriptor is retained for use by Read(), Write(), and related
   * methods. Must be called before any other method except IsInitialized().
   *
   * @retval ErrorCode::kOk    Device opened and configured successfully.
   * @retval ErrorCode::kError Failed to open the device or apply settings.
   */
  hal_interface::ErrorCode Initialize() override;

  /**
   * @brief Closes the tty file descriptor.
   *
   * After this call only Initialize() and IsInitialized() are valid.
   * No further reads or writes can be performed until Initialize() is
   * called again.
   *
   * @retval ErrorCode::kOk    File descriptor closed successfully.
   * @retval ErrorCode::kError close() returned an error.
   */
  hal_interface::ErrorCode Deinitialize() override;

  /**
   * @brief Returns whether Initialize() has been called and succeeded.
   *
   * @retval true  The device is open and ready.
   * @retval false The device is not open.
   */
  bool IsInitialized() override;

  /**
   * @brief Reads bytes from the kernel receive buffer with a timeout.
   *
   * Uses poll() to wait up to @p timeout_ms milliseconds for data.
   * Copies available bytes into @p buffer; may return fewer bytes than
   * @p buffer.size() if fewer arrived before the timeout. In that case
   * the caller should issue another Read() to retrieve remaining data.
   * This is a buffered read: the Linux kernel buffers incoming bytes
   * between Read() calls.
   *
   * @param[out] buffer     Destination span for received bytes.
   * @param[out] bytes_read Actual number of bytes placed in @p buffer.
   * @param[in]  timeout_ms Maximum wait time in milliseconds.
   * @retval ErrorCode::kOk      At least one byte was read.
   * @retval ErrorCode::kTimeout No bytes arrived within @p timeout_ms.
   * @retval ErrorCode::kError   read() or poll() failed.
   */
  hal_interface::ErrorCode Read(std::span<uint8_t> buffer, uint16_t& bytes_read,
                                uint32_t timeout_ms) override;
  /**
   * @brief Reads characters from the kernel receive buffer with a timeout.
   *
   * Converts the character span to a uint8_t span and calls the primary Read()
   * method.
   *
   * @param[out] buffer     Destination span for received characters.
   * @param[out] bytes_read Actual number of characters placed in @p buffer.
   * @param[in]  timeout_ms Maximum wait time in milliseconds.
   * @retval ErrorCode::kOk      At least one character was read.
   * @retval ErrorCode::kTimeout No characters arrived within @p timeout_ms.
   * @retval ErrorCode::kError   read() or poll() failed.
   */
  hal_interface::ErrorCode Read(std::span<char> buffer, uint16_t& bytes_read,
                                uint32_t timeout_ms) override;

  /**
   * @brief Transmits all bytes in @p buffer over the UART line.
   *
   * This method calls the write() syscall, which copies bytes from @p buffer to
   * the kernel transmit buffer. If the parameter wait_end_of_transmission_ set
   * through the constructor is true, Write() will block until the hardware has
   * completed transmission of all bytes through the wire. If false, Write()
   * will return as soon as the bytes have been accepted into the kernel buffer.
   * The data may not be fully transmitted by the time Write() returns, in
   * which case ErrorCode::kTimeout will be returned.
   *
   * @param[in] buffer     Source span of bytes to transmit.
   * @param[in] timeout_ms Maximum wait time in milliseconds.
   * @retval ErrorCode::kOk      All bytes were copied and, if requested,
   * transmitted successfully.
   * @retval ErrorCode::kTimeout The copy / transmission did not complete within
   * @p timeout_ms.
   * @retval ErrorCode::kError   write() or transmit drain failed.
   */
  hal_interface::ErrorCode Write(std::span<const uint8_t> buffer,
                                 uint32_t timeout_ms) override;

  /**
   * @brief Transmits all characters in @p buffer over the UART line.
   *
   * Converts the character span to a uint8_t span and calls the primary Write()
   * method.
   *
   * @param buffer     Source span of characters to transmit.
   * @param timeout_ms Maximum wait time in milliseconds.
   * @retval ErrorCode::kOk      All characters transmitted successfully.
   * @retval ErrorCode::kTimeout Transmission did not finish in time.
   * @retval ErrorCode::kError   write() or poll() failed.
   */
  hal_interface::ErrorCode Write(std::span<const char> buffer,
                                 uint32_t timeout_ms) override;

  /**
   * @brief Returns the number of bytes pending in the kernel receive buffer.
   *
   * Uses ioctl(FIONREAD) to query the kernel. May be used to avoid blocking in
   * Read() when there are not enough bytes available in the internal buffer.
   *
   * @retval Number of bytes available to read without blocking.
   */
  uint16_t BytesAvailable() override;

  /**
   * @brief Discards all bytes in the kernel receive buffer.
   *
   * Calls tcflush(fd_, TCIFLUSH) to purge bytes that arrived but have
   * not yet been consumed by Read().
   *
   * @retval ErrorCode::kOk    Buffer flushed successfully.
   * @retval ErrorCode::kError tcflush() returned an error.
   */
  hal_interface::ErrorCode ClearReadBuffer() override;

 private:
  std::string device_;             ///< Path to the tty device.
  uint32_t baud_rate_;             ///< Desired baud rate.
  bool wait_end_of_transmission_;  ///< Whether Write() should block until
                                   ///< transmission is complete.
  hal_interface::Serial::FlowControl flow_control_;  ///< Flow control setting.
  hal_interface::Serial::DataBits data_bits_;        ///< Number of data bits.
  hal_interface::Serial::Parity parity_;             ///< Parity mode.
  hal_interface::Serial::StopBits stop_bits_;        ///< Number of stop bits.
  int fd_{-1};               ///< File descriptor; -1 when not open.
  bool initialized_{false};  ///< True after successful Initialize().

  // Time-unit conversion constants.
  static const int32_t kMsPerSecond = 1000U;
  static const int8_t kBitsPerByte{10};
};

}  // namespace sfw::hal_linux

#endif  // HAL_LINUX_UART_UART_SERIAL_HPP
