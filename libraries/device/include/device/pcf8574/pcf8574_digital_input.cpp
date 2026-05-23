// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "pcf8574_digital_input.hpp"

namespace sfw::device::pcf8574 {

DigitalInput::DigitalInput(Pcf8574 &expander, uint8_t pin)
  : expander_(expander), pin_(pin) {
}

hal_interface::ErrorCode DigitalInput::Initialize() {
  if (initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  hal_interface::ErrorCode result = expander_.Initialize();
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = expander_.WritePin(pin_, true, kTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  initialized_ = true;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode DigitalInput::Deinitialize() {
  initialized_ = false;
  return hal_interface::ErrorCode::kOk;
}

bool DigitalInput::IsInitialized() {
  return initialized_;
}

hal_interface::ErrorCode DigitalInput::Read(bool &state, uint32_t timeout_ms) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }

  return expander_.ReadPin(pin_, state, timeout_ms);
}

}  // namespace sfw::device::pcf8574
