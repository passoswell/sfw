// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "device/serial_eeprom/serial_eeprom_i2c_transport.hpp"

namespace sfw::device::serial_eeprom {

namespace {

constexpr uint16_t kSerialEepromI2cMax10BitAddress{0x3FFU};
constexpr uint8_t kMaxI2cRegisterAddressSizeBytes{4U};

}  // namespace

SerialEepromI2cTransport::SerialEepromI2cTransport(
    hal_interface::I2cController& i2c, uint16_t target_address,
    bool is_10bit_address, hal_interface::SoftwareTimer& timer,
    uint32_t ready_poll_interval_ms, uint32_t ready_poll_attempt_timeout_ms)
  : i2c_(i2c)
  , target_address_(target_address)
  , is_10bit_address_(is_10bit_address)
  , timer_(timer)
  , ready_poll_interval_ms_(ready_poll_interval_ms)
  , ready_poll_attempt_timeout_ms_(ready_poll_attempt_timeout_ms) {
}

hal_interface::ErrorCode SerialEepromI2cTransport::Initialize() {
  if (initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  hal_interface::ErrorCode result{};

  result = i2c_.Initialize();
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = timer_.Initialize();
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  if (target_address_ > kSerialEepromI2cMax10BitAddress) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }
  if (ready_poll_attempt_timeout_ms_ == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }

  initialized_ = true;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode SerialEepromI2cTransport::Deinitialize() {
  if (!initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  hal_interface::ErrorCode result = timer_.Deinitialize();
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  initialized_ = false;
  return hal_interface::ErrorCode::kOk;
}

bool SerialEepromI2cTransport::IsInitialized() {
  return initialized_;
}

hal_interface::ErrorCode SerialEepromI2cTransport::WriteToAddress(
    uint32_t address, uint8_t address_size_bytes,
    std::span<const uint8_t> buffer, uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }

  return i2c_.WriteRegister(target_address_, is_10bit_address_, address,
                            address_size_bytes, buffer, timeout_ms);
}

hal_interface::ErrorCode SerialEepromI2cTransport::ReadFromAddress(
    uint32_t address, uint8_t address_size_bytes, std::span<uint8_t> buffer,
    uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }

  return i2c_.ReadRegister(target_address_, is_10bit_address_, address,
                           address_size_bytes, buffer, timeout_ms);
}

hal_interface::ErrorCode SerialEepromI2cTransport::WaitUntilReady(
    uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (timeout_ms == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }
  if (ready_poll_attempt_timeout_ms_ == 0U) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }

  hal_interface::ErrorCode result =
      timer_.Start(timeout_ms, hal_interface::TimeUnit::kMilliseconds);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  do {
    // EEPROM devices typically NACK when they are busy, so this Detect() call
    // is used to poll the device until it is ready for the next operation.
    const hal_interface::ErrorCode DetectResult = i2c_.Detect(
        target_address_, is_10bit_address_, ready_poll_attempt_timeout_ms_);
    if (DetectResult == hal_interface::ErrorCode::kOk) {
      return hal_interface::ErrorCode::kOk;
    }

    if (ready_poll_interval_ms_ == 0U) {
      continue;
    }

    result = timer_.Delay(ready_poll_interval_ms_,
                          hal_interface::TimeUnit::kMilliseconds);
    if (result != hal_interface::ErrorCode::kOk) {
      return result;
    }
  } while (!timer_.HasExpired());

  return hal_interface::ErrorCode::kTimeout;
}

}  // namespace sfw::device::serial_eeprom
