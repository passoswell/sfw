// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "hal_linux/stdio/stdio_serial.hpp"

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdint>
#include <span>
#include <thread>

namespace sfw::hal_linux {

namespace {

uint64_t GetStdioNowTimeMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

int ComputeStdioPollTimeoutMs(uint32_t timeout_ms, uint64_t start_time_ms) {
  if (timeout_ms > static_cast<uint32_t>(INT_MAX)) {
    return INT_MAX;
  }

  const uint64_t Now = GetStdioNowTimeMs();
  const uint64_t Elapsed = Now - start_time_ms;
  if (Elapsed >= timeout_ms) {
    return 0;
  }

  return static_cast<int>(timeout_ms - Elapsed);
}

}  // namespace

StdioSerial::StdioSerial(bool wait_end_of_transmission)
  : wait_end_of_transmission_(wait_end_of_transmission) {
}

StdioSerial::~StdioSerial() {
  (void)RestoreConfiguration();
}

hal_interface::ErrorCode StdioSerial::Initialize() {
  if (initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  if (::tcgetattr(STDIN_FILENO, &terminal_original_) != 0) {
    return hal_interface::ErrorCode::kError;
  }
  terminal_saved_ = true;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  stdin_original_flags_ = ::fcntl(STDIN_FILENO, F_GETFL, 0);
  if (stdin_original_flags_ < 0) {
    return RestoreConfiguration();
  }
  stdin_flags_saved_ = true;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  stdout_original_flags_ = ::fcntl(STDOUT_FILENO, F_GETFL, 0);
  if (stdout_original_flags_ < 0) {
    return RestoreConfiguration();
  }
  stdout_flags_saved_ = true;

  struct termios raw_mode = terminal_original_;
  ::cfmakeraw(&raw_mode);
  if (::tcsetattr(STDIN_FILENO, TCSANOW, &raw_mode) != 0) {
    return RestoreConfiguration();
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (::fcntl(STDIN_FILENO, F_SETFL, stdin_original_flags_ | O_NONBLOCK) != 0) {
    return RestoreConfiguration();
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (::fcntl(STDOUT_FILENO, F_SETFL, stdout_original_flags_ | O_NONBLOCK)
      != 0) {
    return RestoreConfiguration();
  }

  initialized_ = true;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode StdioSerial::Deinitialize() {
  return RestoreConfiguration();
}

bool StdioSerial::IsInitialized() {
  return initialized_;
}

hal_interface::ErrorCode StdioSerial::Read(std::span<uint8_t> buffer,
                                           uint16_t& bytes_read,
                                           uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }

  bytes_read = 0;
  if (buffer.empty()) {
    return hal_interface::ErrorCode::kOk;
  }

  const uint64_t StartTimeMs = GetStdioNowTimeMs();
  struct pollfd read_poll {};
  read_poll.fd = STDIN_FILENO;
  read_poll.events = POLLIN;

  size_t bytes_offset = 0;
  while (bytes_offset < buffer.size()) {
    const int PollTimeoutMs =
        ComputeStdioPollTimeoutMs(timeout_ms, StartTimeMs);
    const int PollResult = ::poll(&read_poll, 1, PollTimeoutMs);
    if (PollResult < 0) {
      if (errno == EINTR) {
        continue;
      }
      return hal_interface::ErrorCode::kError;
    }

    if (PollResult == 0) {
      break;
    }

    if ((read_poll.revents & (POLLERR | POLLNVAL)) != 0) {
      return hal_interface::ErrorCode::kError;
    }

    const ssize_t ReadResult =
        ::read(STDIN_FILENO, buffer.data() + bytes_offset,
               buffer.size() - bytes_offset);
    if (ReadResult < 0) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }
      return hal_interface::ErrorCode::kError;
    }

    if (ReadResult == 0) {
      break;
    }

    bytes_offset += static_cast<size_t>(ReadResult);
  }

  bytes_read = static_cast<uint16_t>(bytes_offset);
  if (bytes_read == 0) {
    return hal_interface::ErrorCode::kTimeout;
  }
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode StdioSerial::Read(std::span<char> buffer,
                                           uint16_t& bytes_read,
                                           uint32_t timeout_ms) {
  std::span<uint8_t> uint8_t_span{
      reinterpret_cast<uint8_t*>(buffer.data()),  // NOLINT
      buffer.size()};
  return Read(uint8_t_span, bytes_read, timeout_ms);
}

hal_interface::ErrorCode StdioSerial::Write(std::span<const uint8_t> buffer,
                                            uint32_t timeout_ms) {
  ssize_t num_bytes = ::write(STDOUT_FILENO, buffer.data(), buffer.size());
  if (num_bytes <= 0) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0 || !wait_end_of_transmission_) {
    return hal_interface::ErrorCode::kOk;
  }
  if (timeout_ms > INT32_MAX) {
    timeout_ms = INT32_MAX;
  }

  int pending{};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  ::ioctl(STDOUT_FILENO, TIOCOUTQ, &pending);
  int32_t total_wait_time_ms =
      (pending * kBitsPerByte * kMsPerSecond) / static_cast<int32_t>(kBaudRate);

  while (total_wait_time_ms > 0 && pending > 0) {
    if (total_wait_time_ms >= static_cast<int32_t>(timeout_ms)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
      total_wait_time_ms = 0;
      timeout_ms = 0;
    } else {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(total_wait_time_ms));

      timeout_ms -= total_wait_time_ms;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    ::ioctl(STDOUT_FILENO, TIOCOUTQ, &pending);
    if (pending > 0) {
      if (timeout_ms > 0) {
        total_wait_time_ms = (pending * kBitsPerByte * kMsPerSecond)
                             / static_cast<int32_t>(kBaudRate);
      } else {
        return hal_interface::ErrorCode::kTimeout;
      }
    }
  }
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode StdioSerial::Write(std::span<const char> buffer,
                                            uint32_t timeout_ms) {
  std::span<const uint8_t> uint8_t_span{
      reinterpret_cast<const uint8_t*>(buffer.data()),  // NOLINT
      buffer.size()};
  return Write(uint8_t_span, timeout_ms);
}

uint16_t StdioSerial::BytesAvailable() {
  if (!initialized_) {
    return 0;
  }

  int pending_bytes = 0;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (::ioctl(STDIN_FILENO, FIONREAD, &pending_bytes) < 0) {
    return 0;
  }

  if (pending_bytes < 0) {
    return 0;
  }
  if (pending_bytes > UINT16_MAX) {
    return UINT16_MAX;
  }
  return static_cast<uint16_t>(pending_bytes);
}

hal_interface::ErrorCode StdioSerial::ClearReadBuffer() {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }

  if (::tcflush(STDIN_FILENO, TCIFLUSH) != 0) {
    return hal_interface::ErrorCode::kError;
  }
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode StdioSerial::RestoreConfiguration() {
  bool restore_failed = false;

  if (terminal_saved_) {
    if (::tcsetattr(STDIN_FILENO, TCSANOW, &terminal_original_) != 0) {
      restore_failed = true;
    }
    terminal_saved_ = false;
  }

  if (stdin_flags_saved_) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    if (::fcntl(STDIN_FILENO, F_SETFL, stdin_original_flags_) != 0) {
      restore_failed = true;
    }
    stdin_flags_saved_ = false;
  }

  if (stdout_flags_saved_) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    if (::fcntl(STDOUT_FILENO, F_SETFL, stdout_original_flags_) != 0) {
      restore_failed = true;
    }
    stdout_flags_saved_ = false;
  }

  initialized_ = false;

  if (restore_failed) {
    return hal_interface::ErrorCode::kError;
  }
  return hal_interface::ErrorCode::kOk;
}

}  // namespace sfw::hal_linux
