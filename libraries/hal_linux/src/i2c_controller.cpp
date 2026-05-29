// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "hal_linux/i2c/i2c_controller.hpp"

#include <fcntl.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <i2c/smbus.h>
#ifdef __cplusplus
}
#endif
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace sfw::hal_linux {

namespace {

constexpr uint16_t kMax7BitAddress = 0x7FU;
constexpr uint8_t kMinRegisterAddressSizeBytes = 1U;
constexpr uint8_t kMaxRegisterAddressSizeBytes = 4U;
constexpr uint32_t kMsPerI2CTimeoutTick = 10U;
constexpr std::size_t kMaxSafeCombinedReadBytes = 4U;

std::array<uint8_t, kMaxRegisterAddressSizeBytes> EncodeRegisterAddress(
    uint32_t register_address, uint8_t register_size_bytes) {
  std::array<uint8_t, kMaxRegisterAddressSizeBytes> encoded{};
  for (uint8_t index = 0U; index < register_size_bytes; index++) {
    const auto Shift =
        static_cast<uint8_t>((register_size_bytes - 1U - index) * 8U);
    encoded.at(index) = static_cast<uint8_t>(register_address >> Shift);
  }
  return encoded;
}

}  // namespace

I2cController::I2cController(std::string device) : device_{std::move(device)} {
}

I2cController::~I2cController() {
  (void)hal_linux::I2cController::Deinitialize();
}

hal_interface::ErrorCode I2cController::Initialize() {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  fd_ = ::open(device_.c_str(), O_RDWR);
  if (fd_ < 0) {
    return hal_interface::ErrorCode::kError;
  }

  initialized_ = true;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode I2cController::Deinitialize() {
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

bool I2cController::IsInitialized() {
  return initialized_;
}

hal_interface::ErrorCode I2cController::Read(uint16_t target_address,
                                             bool is_10bit_address,
                                             std::span<uint8_t> buffer,
                                             uint32_t timeout_ms) {
  if (!initialized_ || timeout_ms == 0) {
    return hal_interface::ErrorCode::kError;
  }

  if (SelectTarget(target_address, is_10bit_address)
      != hal_interface::ErrorCode::kOk) {
    return hal_interface::ErrorCode::kError;
  }

  if (ConfigureTransactionTimeout(timeout_ms)
      != hal_interface::ErrorCode::kOk) {
    return hal_interface::ErrorCode::kError;
  }

  const ssize_t BytesRead = ::read(fd_, buffer.data(), buffer.size());
  if (BytesRead < 0) {
    return hal_interface::ErrorCode::kError;
  }

  if (BytesRead != static_cast<ssize_t>(buffer.size())) {
    return hal_interface::ErrorCode::kTimeout;
  }

  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode I2cController::Write(uint16_t target_address,
                                              bool is_10bit_address,
                                              std::span<const uint8_t> buffer,
                                              uint32_t timeout_ms) {
  if (!initialized_ || timeout_ms == 0) {
    return hal_interface::ErrorCode::kError;
  }

  if (SelectTarget(target_address, is_10bit_address)
      != hal_interface::ErrorCode::kOk) {
    return hal_interface::ErrorCode::kError;
  }

  if (ConfigureTransactionTimeout(timeout_ms)
      != hal_interface::ErrorCode::kOk) {
    return hal_interface::ErrorCode::kError;
  }

  const ssize_t BytesWritten = ::write(fd_, buffer.data(), buffer.size());
  if (BytesWritten < 0) {
    return hal_interface::ErrorCode::kError;
  }

  if (BytesWritten != static_cast<ssize_t>(buffer.size())) {
    return hal_interface::ErrorCode::kTimeout;
  }

  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode I2cController::WriteThenRead(
    uint16_t target_address, bool is_10bit_address,
    std::span<const uint8_t> write_buffer, std::span<uint8_t> read_buffer,
    uint32_t timeout_ms) {
  if (!initialized_ || timeout_ms == 0) {
    return hal_interface::ErrorCode::kError;
  }

  if (SelectTarget(target_address, is_10bit_address)
      != hal_interface::ErrorCode::kOk) {
    return hal_interface::ErrorCode::kError;
  }

  if (ConfigureTransactionTimeout(timeout_ms)
      != hal_interface::ErrorCode::kOk) {
    return hal_interface::ErrorCode::kError;
  }

  if (read_buffer.size() > kMaxSafeCombinedReadBytes) {
    hal_interface::ErrorCode result =
        Write(target_address, is_10bit_address, write_buffer, timeout_ms);
    if (result != hal_interface::ErrorCode::kOk) {
      return result;
    }
    return Read(target_address, is_10bit_address, read_buffer, timeout_ms);
  }

  std::array<i2c_msg, 2U> messages{};
  uint32_t message_count = 0U;

  if (!write_buffer.empty()) {
    i2c_msg& message = messages.at(message_count);
    message.addr = target_address;
    if (is_10bit_address) {
      message.flags |= I2C_M_TEN;
    } else {
      message.flags = 0U;
    }
    message.len = static_cast<__u16>(write_buffer.size());
    message.buf = const_cast<uint8_t*>(write_buffer.data());  // NOLINT
    message_count++;
  }

  if (!read_buffer.empty()) {
    i2c_msg& message = messages.at(message_count);
    message.addr = target_address;
    message.flags = I2C_M_RD;
    if (is_10bit_address) {
      message.flags |= I2C_M_TEN;
    }
    message.len = static_cast<__u16>(read_buffer.size());
    message.buf = read_buffer.data();
    message_count++;
  }

  i2c_rdwr_ioctl_data transfer{};
  transfer.msgs = messages.data();
  transfer.nmsgs = message_count;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (::ioctl(fd_, I2C_RDWR, &transfer) < 0) {
    return hal_interface::ErrorCode::kError;
  }
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode I2cController::ReadRegister(
    uint16_t target_address, bool is_10bit_address, uint32_t register_address,
    uint8_t register_size_bytes, std::span<uint8_t> buffer,
    uint32_t timeout_ms) {
  if (!initialized_ || timeout_ms == 0) {
    return hal_interface::ErrorCode::kError;
  }

  if (register_size_bytes < kMinRegisterAddressSizeBytes
      || register_size_bytes > kMaxRegisterAddressSizeBytes) {
    return hal_interface::ErrorCode::kError;
  }

  const std::array<uint8_t, kMaxRegisterAddressSizeBytes> EncodedAddress =
      EncodeRegisterAddress(register_address, register_size_bytes);
  const std::span<const uint8_t> RegisterBytes(EncodedAddress.data(),
                                               register_size_bytes);

  return WriteThenRead(target_address, is_10bit_address, RegisterBytes, buffer,
                       timeout_ms);
}

hal_interface::ErrorCode I2cController::WriteRegister(
    uint16_t target_address, bool is_10bit_address, uint32_t register_address,
    uint8_t register_size_bytes, std::span<const uint8_t> buffer,
    uint32_t timeout_ms) {
  if (!initialized_ || timeout_ms == 0) {
    return hal_interface::ErrorCode::kError;
  }

  if (register_size_bytes < kMinRegisterAddressSizeBytes
      || register_size_bytes > kMaxRegisterAddressSizeBytes) {
    return hal_interface::ErrorCode::kError;
  }

  const std::array<uint8_t, kMaxRegisterAddressSizeBytes> EncodedAddress =
      EncodeRegisterAddress(register_address, register_size_bytes);

  std::vector<uint8_t> payload(register_size_bytes + buffer.size());
  std::copy_n(EncodedAddress.begin(), register_size_bytes, payload.begin());
  std::copy(buffer.begin(), buffer.end(),
            payload.begin() + register_size_bytes);

  const std::span<const uint8_t> PayloadSpan(payload.data(), payload.size());
  return Write(target_address, is_10bit_address, PayloadSpan, timeout_ms);
}

hal_interface::ErrorCode I2cController::Detect(uint16_t target_address,
                                               bool is_10bit_address,
                                               uint32_t timeout_ms) {
  if (!initialized_ || timeout_ms == 0) {
    return hal_interface::ErrorCode::kError;
  }

  hal_interface::ErrorCode result = ConfigureTransactionTimeout(timeout_ms);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = SelectTarget(target_address, is_10bit_address);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  if (i2c_smbus_read_byte(fd_) < 0) {
    return hal_interface::ErrorCode::kError;
  }

  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode I2cController::ConfigureTransactionTimeout(
    uint32_t timeout_ms) const {
  uint32_t timeout_ticks =
      (timeout_ms + (kMsPerI2CTimeoutTick - 1U)) / kMsPerI2CTimeoutTick;
  if (timeout_ticks == 0U) {
    timeout_ticks = 1U;
  }

  const uint32_t MaxTimeoutTicks =
      std::numeric_limits<int>::max() / kMsPerI2CTimeoutTick;
  timeout_ticks = std::min(timeout_ticks, MaxTimeoutTicks);

  const int TimeoutArg = static_cast<int>(timeout_ticks);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (::ioctl(fd_, I2C_TIMEOUT, TimeoutArg) < 0) {
    return hal_interface::ErrorCode::kError;
  }

  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode I2cController::SelectTarget(
    uint16_t target_address, bool is_10bit_address) const {
  int n_bits_config{};
  if (is_10bit_address) {
    n_bits_config = 1;
  } else {
    n_bits_config = 0;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (::ioctl(fd_, I2C_TENBIT, n_bits_config) < 0) {
    return hal_interface::ErrorCode::kError;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  if (::ioctl(fd_, I2C_SLAVE, static_cast<int>(target_address)) < 0) {
    return hal_interface::ErrorCode::kError;
  }
  return hal_interface::ErrorCode::kOk;
}

}  // namespace sfw::hal_linux
