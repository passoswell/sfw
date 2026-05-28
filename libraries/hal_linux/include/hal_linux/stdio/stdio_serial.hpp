// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_LINUX_STDIO_STDIO_SERIAL_HPP
#define HAL_LINUX_STDIO_STDIO_SERIAL_HPP

#include <termios.h>

#include <cstdint>
#include <span>

#include "hal_interface/serial.hpp"

namespace sfw::hal_linux {

/**
 * @brief STDIO implementation of the hal_interface::Serial interface.
 *
 * Uses process standard input and output as a serial-like byte stream.
 * During operation, stdin is placed in raw mode so input bytes are received
 * immediately without line editing or echo. Raw mode is restored to the
 * previous terminal configuration by Deinitialize() and also by the
 * destructor for safe cleanup.
 *
 * Include path: `#include "hal_linux/stdio/stdio_serial.hpp"`
 *
 * Typical usage:
 * 1. Construct a StdioSerial instance.
 * 2. Call Initialize() to switch stdin into raw mode.
 * 3. Call Write() to send bytes to stdout.
 * 4. Call Read() to receive bytes from stdin.
 * 5. Call Deinitialize() when terminal interaction is complete.
 */
class StdioSerial final : public hal_interface::Serial {
 public:
  /**
   * @brief Constructs a StdioSerial instance.
   *
   * The instance is not initialized until Initialize() is called. By default,
   * the instance is configured to block until the end of transmission in
   * Write(), but this can be changed by passing false to the constructor.
   *
   * @param[in] wait_end_of_transmission If true, Write() will block until all
   * bytes are transmitted.
   */
  explicit StdioSerial(bool wait_end_of_transmission = true);

  /**
   * @brief Destructor
   *
   * Restores terminal settings if Initialize() previously enabled raw mode.
   */
  ~StdioSerial() override;

  StdioSerial(const StdioSerial&) = delete;
  StdioSerial& operator=(const StdioSerial&) = delete;
  StdioSerial(StdioSerial&&) = delete;
  StdioSerial& operator=(StdioSerial&&) = delete;

  /**
   * @brief Initializes stdin/stdout for serial-like operation.
   *
   * Saves the current stdin terminal settings, switches stdin to raw mode,
   * and applies non-blocking file status flags to stdin/stdout so timeout
   * handling can be enforced with poll().
   *
   * @retval ErrorCode::kOk    Configuration completed successfully.
   * @retval ErrorCode::kError Failed to read or apply terminal settings.
   */
  hal_interface::ErrorCode Initialize() override;

  /**
   * @brief Restores terminal settings and file status flags.
   *
   * Reverts stdin terminal attributes and stdin/stdout non-blocking flags
   * saved during Initialize().
   *
   * @retval ErrorCode::kOk    Previous settings restored successfully.
   * @retval ErrorCode::kError Restoration failed.
   */
  hal_interface::ErrorCode Deinitialize() override;

  /**
   * @brief Reports whether Initialize() completed successfully.
   *
   * @retval true  STDIO backend is initialized and ready.
   * @retval false STDIO backend is not initialized.
   */
  bool IsInitialized() override;

  /**
   * @brief Reads bytes from stdin with timeout handling.
   *
   * Polls stdin for readable data and copies bytes into @p buffer until the
   * buffer is full or @p timeout_ms expires. This method fulfills the
   * interface contract by updating @p bytes_read with the amount copied.
   *
   * @param[out] buffer     Destination span for received bytes.
   * @param[out] bytes_read Number of bytes written into @p buffer.
   * @param[in]  timeout_ms Maximum time to wait for data in milliseconds.
   * @retval ErrorCode::kOk      At least one byte was read.
   * @retval ErrorCode::kTimeout No bytes arrived before timeout.
   * @retval ErrorCode::kError   Poll or read failed.
   */
  hal_interface::ErrorCode Read(std::span<uint8_t> buffer, uint16_t& bytes_read,
                                uint32_t timeout_ms) override;

  /**
   * @brief Reads characters from stdin with timeout handling.
   *
   * Implements the character overload by forwarding to the byte overload,
   * preserving timeout behavior and @p bytes_read semantics.
   *
   * @param[out] buffer     Destination span for received characters.
   * @param[out] bytes_read Number of characters written into @p buffer.
   * @param[in]  timeout_ms Maximum time to wait for data in milliseconds.
   * @retval ErrorCode::kOk      At least one character was read.
   * @retval ErrorCode::kTimeout No characters arrived before timeout.
   * @retval ErrorCode::kError   Poll or read failed.
   */
  hal_interface::ErrorCode Read(std::span<char> buffer, uint16_t& bytes_read,
                                uint32_t timeout_ms) override;

  /**
   * @brief Writes bytes to stdout with timeout handling.
   *
   * Copies bytes from @p buffer to stdout and, when the instance was
   * constructed with wait_end_of_transmission enabled, waits until the data
   * has been transmitted by the terminal or pty driver. The call may return
   * before physical transmission only when wait_end_of_transmission is false.
   * The timeout applies to the full write and transmit sequence.
   *
   * @param[in] buffer     Source span containing bytes to transmit.
   * @param[in] timeout_ms Maximum time to complete transmission.
   * @retval ErrorCode::kOk      All bytes were written and, if requested,
   * transmission completed.
   * @retval ErrorCode::kTimeout Timeout elapsed before completion.
   * @retval ErrorCode::kError   Write or transmit confirmation failed.
   */
  hal_interface::ErrorCode Write(std::span<const uint8_t> buffer,
                                 uint32_t timeout_ms) override;

  /**
   * @brief Writes characters to stdout with timeout handling.
   *
   * Implements the character overload by forwarding to the byte overload,
   * preserving timeout behavior and transmit completion semantics.
   *
   * @param[in] buffer     Source span containing characters to transmit.
   * @param[in] timeout_ms Maximum time to complete transmission.
   * @retval ErrorCode::kOk      All characters were written and, if
   * requested, transmission completed.
   * @retval ErrorCode::kTimeout Timeout elapsed before completion.
   * @retval ErrorCode::kError   Write or transmit confirmation failed.
   */
  hal_interface::ErrorCode Write(std::span<const char> buffer,
                                 uint32_t timeout_ms) override;

  /**
   * @brief Returns immediately available bytes on stdin.
   *
   * Uses ioctl(FIONREAD) to report queued input bytes.
   *
   * @retval Number of bytes available for non-blocking read.
   */
  uint16_t BytesAvailable() override;

  /**
   * @brief Clears pending bytes from stdin input queue.
   *
   * Discards unread input by draining currently available bytes.
   *
   * @retval ErrorCode::kOk    Input queue cleared.
   * @retval ErrorCode::kError Clearing the queue failed.
   */
  hal_interface::ErrorCode ClearReadBuffer() override;

 private:
  // Performs restoration without relying on virtual calls.
  hal_interface::ErrorCode RestoreConfiguration();

  bool wait_end_of_transmission_;   ///< Whether Write() should block until
                                    ///< transmission is complete.
  bool initialized_{false};         ///< True after successful Initialize().
  bool terminal_saved_{false};      ///< True when stdin termios was captured.
  bool stdin_flags_saved_{false};   ///< True when stdin flags were captured.
  bool stdout_flags_saved_{false};  ///< True when stdout flags were captured.

  struct termios terminal_original_ {};

  int stdin_original_flags_{0};   ///< Saved stdin fcntl flags.
  int stdout_original_flags_{0};  ///< Saved stdout fcntl flags.

  // Time-unit conversion constants.
  static const int32_t kMsPerSecond = 1000U;
  static const int8_t kBitsPerByte{10};
  static const int32_t kBaudRate{115200};
};

}  // namespace sfw::hal_linux

#endif  // HAL_LINUX_STDIO_STDIO_SERIAL_HPP