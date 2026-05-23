// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "pcf8574.hpp"

#include <span>

namespace sfw::device::pcf8574 {

Pcf8574::Pcf8574(hal_interface::I2cController &i2c, uint8_t address,
                 uint8_t initial_port_state)
  : i2c_(i2c), address_(address), initial_port_state_(initial_port_state) {
}

hal_interface::ErrorCode Pcf8574::Initialize() {
  if (initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  auto result = hal_interface::ErrorCode::kOk;

  if (!i2c_.IsInitialized()) {
    result = i2c_.Initialize();
    if (result != hal_interface::ErrorCode::kOk) {
      return result;
    }
  }

  result = i2c_.Detect(address_, false, kI2cTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }
  initialized_ = true;

  result = WritePort(initial_port_state_, kI2cTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    initialized_ = false;
  }
  port_state_ = initial_port_state_;
  return result;
}

hal_interface::ErrorCode Pcf8574::ReadPin(uint8_t pin, bool &level,
                                          uint32_t timeout_ms) {
  if (pin >= kPinCount) {
    return hal_interface::ErrorCode::kOutOfRange;
  }

  uint8_t port_value{};
  const hal_interface::ErrorCode Error = ReadPort(port_value, timeout_ms);
  if (Error != hal_interface::ErrorCode::kOk) {
    return Error;
  }

  level = ((port_value >> pin) & 0x01U) != 0U;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode Pcf8574::WritePin(uint8_t pin, bool level,
                                           uint32_t timeout_ms) {
  if (pin >= kPinCount) {
    return hal_interface::ErrorCode::kOutOfRange;
  }

  // uint8_t port_value{};
  // hal_interface::ErrorCode error = ReadPort(port_value, timeout_ms);
  // if (error != hal_interface::ErrorCode::kOk) {
  //   return error;
  // }

  const auto PinMask = static_cast<uint8_t>(1U << pin);
  if (level) {
    port_state_ = static_cast<uint8_t>(port_state_ | PinMask);
  } else {
    port_state_ =
        static_cast<uint8_t>(port_state_ & static_cast<uint8_t>(~PinMask));
  }

  return WritePort(port_state_, timeout_ms);
}

hal_interface::ErrorCode Pcf8574::ReadPort(uint8_t &value,
                                           uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }

  std::span<uint8_t> read_buffer(&value, 1);
  return i2c_.Read(address_, false, read_buffer, timeout_ms);
}

hal_interface::ErrorCode Pcf8574::WritePort(uint8_t value,
                                            uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }

  port_state_ = value;
  const std::span<const uint8_t> WriteBuffer(&value, 1);
  return i2c_.Write(address_, false, WriteBuffer, timeout_ms);
}

}  // namespace sfw::device::pcf8574
