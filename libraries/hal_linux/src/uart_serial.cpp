// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "hal_linux/uart/uart_serial.hpp"

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <climits>
#include <cstdint>
#include <span>
#include <thread>

namespace sfw::hal_linux {

namespace {

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
speed_t BaudRateToSpeed(uint32_t baud_rate) {
  switch (baud_rate) {
    case 50U:
      return B50;
    case 75U:
      return B75;
    case 110U:
      return B110;
    case 134U:
      return B134;
    case 150U:
      return B150;
    case 200U:
      return B200;
    case 300U:
      return B300;
    case 600U:
      return B600;
    case 1200U:
      return B1200;
    case 1800U:
      return B1800;
    case 2400U:
      return B2400;
    case 4800U:
      return B4800;
    case 9600U:
      return B9600;
    case 19200U:
      return B19200;
    case 38400U:
      return B38400;
    case 57600U:
      return B57600;
    case 115200U:
      return B115200;
    case 230400U:
      return B230400;
    case 460800U:
      return B460800;
    case 500000U:
      return B500000;
    case 576000U:
      return B576000;
    case 921600U:
      return B921600;
    case 1000000U:
      return B1000000;
    case 1152000U:
      return B1152000;
    case 1500000U:
      return B1500000;
    case 2000000U:
      return B2000000;
    case 2500000U:
      return B2500000;
    case 3000000U:
      return B3000000;
    case 3500000U:
      return B3500000;
    case 4000000U:
      return B4000000;
    default:
      return B0;
  }
}
// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

tcflag_t DataBitsToFlag(hal_interface::Serial::DataBits data_bits) {
  switch (data_bits) {
    case hal_interface::Serial::DataBits::k5:
      return CS5;
    case hal_interface::Serial::DataBits::k6:
      return CS6;
    case hal_interface::Serial::DataBits::k7:
      return CS7;
    case hal_interface::Serial::DataBits::k8:
    default:
      return CS8;
  }
}

uint64_t GetNowTimeMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

}  // namespace

UartSerial::UartSerial(std::string device, uint32_t baud_rate,
                       bool wait_end_of_transmission,
                       hal_interface::Serial::FlowControl flow_control,
                       hal_interface::Serial::DataBits data_bits,
                       hal_interface::Serial::Parity parity,
                       hal_interface::Serial::StopBits stop_bits)
  : device_{std::move(device)}
  , baud_rate_{baud_rate}
  , wait_end_of_transmission_{wait_end_of_transmission}
  , flow_control_{flow_control}
  , data_bits_{data_bits}
  , parity_{parity}
  , stop_bits_{stop_bits} {
}

UartSerial::~UartSerial() {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

hal_interface::ErrorCode UartSerial::Initialize() {
  if (initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  speed_t speed = BaudRateToSpeed(baud_rate_);
  if (speed == B0) {
    return hal_interface::ErrorCode::kError;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  fd_ = ::open(device_.c_str(), O_RDWR | O_NOCTTY);
  if (fd_ < 0) {
    return hal_interface::ErrorCode::kError;
  }

  struct termios tty {};
  if (::tcgetattr(fd_, &tty) != 0) {
    ::close(fd_);
    fd_ = -1;
    return hal_interface::ErrorCode::kError;
  }
  ::cfmakeraw(&tty);

  // Set baud rate
  ::cfsetispeed(&tty, speed);
  ::cfsetospeed(&tty, speed);

  tty.c_cflag &= ~CSIZE;  // Clear all the size bits
  tty.c_cflag |= DataBitsToFlag(data_bits_);

  if (parity_ == Parity::kOdd) {
    tty.c_cflag |= PARENB;
    tty.c_cflag |= PARODD;
  } else if (parity_ == Parity::kEven) {
    tty.c_cflag |= PARENB;
    tty.c_cflag &= ~PARODD;
  } else {
    tty.c_cflag &= ~PARENB;
  }

  if (stop_bits_ == StopBits::k2) {
    tty.c_cflag |= CSTOPB;
  } else {
    tty.c_cflag &= ~CSTOPB;
  }

  if (flow_control_ == hal_interface::Serial::FlowControl::kRtsCts) {
    tty.c_cflag |= CRTSCTS;  // Enable RTS/CTS hardware flow control
  } else {
    tty.c_cflag &= ~CRTSCTS;  // No hardware flow control
  }

  tty.c_cflag |= CREAD;   // Turn on READ
  tty.c_cflag |= CLOCAL;  // Ignore ctrl lines (CLOCAL = 1)

  // Configuring uart into nonblocking mode
  // ::read() will return immediately with -1 if no data is available
  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 0;

  if (::tcsetattr(fd_, TCSANOW, &tty) != 0) {
    ::close(fd_);
    fd_ = -1;
    return hal_interface::ErrorCode::kError;
  }
  tcflush(fd_, TCIFLUSH);
  tcflush(fd_, TCIFLUSH);
  tcsetattr(fd_, TCSANOW, &tty);

  initialized_ = true;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode UartSerial::Deinitialize() {
  if (!initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  if (::close(fd_) != 0) {
    return hal_interface::ErrorCode::kError;
  }

  fd_ = -1;
  initialized_ = false;
  return hal_interface::ErrorCode::kOk;
}

bool UartSerial::IsInitialized() {
  return initialized_;
}

hal_interface::ErrorCode UartSerial::Read(std::span<uint8_t> buffer,
                                          uint16_t& bytes_read,
                                          uint32_t timeout_ms) {
  uint64_t now_time = GetNowTimeMs();
  int poll_timeout_ms = (timeout_ms <= static_cast<uint32_t>(INT_MAX))
                            ? static_cast<int>(timeout_ms)
                            : INT_MAX;
  uint16_t bytes_remaining = buffer.size();
  uint16_t poll_bytes_read{0};
  struct pollfd read_poll {};
  read_poll.fd = fd_;
  read_poll.events = POLLIN;

  bytes_read = 0;

  while (poll_bytes_read < buffer.size() && poll_timeout_ms > 0) {
    // Poll for one byte available or timeout
    if (::poll(&read_poll, 1, poll_timeout_ms) < 0) {
      return hal_interface::ErrorCode::kError;
    }

    // Read available bytes into the buffer and update counters
    ssize_t num_bytes =
        ::read(fd_, buffer.subspan(poll_bytes_read).data(), bytes_remaining);
    if (num_bytes < 0) {
      return hal_interface::ErrorCode::kError;
    }
    if (num_bytes > 0) {
      poll_bytes_read += static_cast<uint16_t>(num_bytes);
      bytes_remaining -= poll_bytes_read;
    }

    // Update Remaining timeout
    uint64_t current_time = GetNowTimeMs();
    poll_timeout_ms -= static_cast<int>(current_time - now_time);
    now_time = current_time;
  }

  bytes_read = poll_bytes_read;
  if (poll_bytes_read == 0) {
    return hal_interface::ErrorCode::kTimeout;
  }
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode UartSerial::Read(std::span<char> buffer,
                                          uint16_t& bytes_read,
                                          uint32_t timeout_ms) {
  std::span<uint8_t> uint8_t_span{
      reinterpret_cast<uint8_t*>(buffer.data()),  // NOLINT
      buffer.size()};
  return Read(uint8_t_span, bytes_read, timeout_ms);
}

hal_interface::ErrorCode UartSerial::Write(std::span<const uint8_t> buffer,
                                           uint32_t timeout_ms) {
  (void)timeout_ms;
  ssize_t num_bytes = ::write(fd_, buffer.data(), buffer.size());
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
  ::ioctl(fd_, TIOCOUTQ, &pending);
  int32_t total_wait_time_ms = (pending * kBitsPerByte * kMsPerSecond)
                               / static_cast<int32_t>(baud_rate_);

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
    ::ioctl(fd_, TIOCOUTQ, &pending);
    if (pending > 0) {
      if (timeout_ms > 0) {
        total_wait_time_ms =
            (pending * kBitsPerByte) / static_cast<int32_t>(baud_rate_);
      } else {
        return hal_interface::ErrorCode::kTimeout;
      }
    }
  }
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode UartSerial::Write(std::span<const char> buffer,
                                           uint32_t timeout_ms) {
  std::span<const uint8_t> uint8_t_span{
      reinterpret_cast<const uint8_t*>(buffer.data()),  // NOLINT
      buffer.size()};
  return Write(uint8_t_span, timeout_ms);
}

uint16_t UartSerial::BytesAvailable() {
  int pending_bytes = 0;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (::ioctl(fd_, FIONREAD, &pending_bytes) < 0) {
    return 0U;
  }
  return static_cast<uint16_t>(pending_bytes);
}

hal_interface::ErrorCode UartSerial::ClearReadBuffer() {
  if (::tcflush(fd_, TCIFLUSH) != 0) {
    return hal_interface::ErrorCode::kError;
  }
  return hal_interface::ErrorCode::kOk;
}

}  // namespace sfw::hal_linux
