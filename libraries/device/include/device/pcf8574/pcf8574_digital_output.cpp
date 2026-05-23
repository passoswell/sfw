// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "pcf8574_digital_output.hpp"

namespace sfw::device::pcf8574 {

DigitalOutput::DigitalOutput(Pcf8574 &expander, uint8_t pin, bool initial_state)
  : expander_(expander), pin_(pin), initial_state_(initial_state) {
}

hal_interface::ErrorCode DigitalOutput::Initialize() {
  if (initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  hal_interface::ErrorCode result = expander_.Initialize();
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = expander_.WritePin(pin_, initial_state_, kTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  state_ = initial_state_;
  initialized_ = true;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode DigitalOutput::Deinitialize() {
  auto result = expander_.WritePin(pin_, initial_state_, kTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  initialized_ = false;
  return hal_interface::ErrorCode::kOk;
}

bool DigitalOutput::IsInitialized() {
  return initialized_;
}

hal_interface::ErrorCode DigitalOutput::Write(bool state, uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }

  const hal_interface::ErrorCode Result =
      expander_.WritePin(pin_, state, timeout_ms);
  if (Result != hal_interface::ErrorCode::kOk) {
    return Result;
  }

  state_ = state;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode DigitalOutput::Toggle(uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }

  return Write(!state_, timeout_ms);
}

}  // namespace sfw::device::pcf8574
