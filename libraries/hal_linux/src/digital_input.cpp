// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "hal_linux/dio/digital_input.hpp"

#include <gpiod.h>

#include <cstdint>
#include <string>
#include <utility>

namespace sfw::hal_linux {

DigitalInput::DigitalInput(std::string device, uint32_t line_offset,
                           hal_interface::DigitalInput::LineBias bias)
  : device_{std::move(device)}, line_offset_{line_offset}, bias_{bias} {
}

DigitalInput::~DigitalInput() {
  (void)CloseLineAndChip();
}

hal_interface::ErrorCode DigitalInput::Initialize() {
  if (initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  if (device_.starts_with(KDevPathPrefix)) {
    chip_ = ::gpiod_chip_open(device_.c_str());
  } else {
    chip_ = ::gpiod_chip_open_by_name(device_.c_str());
  }

  if (chip_ == nullptr) {
    return hal_interface::ErrorCode::kError;
  }

  line_ = ::gpiod_chip_get_line(chip_, line_offset_);
  if (line_ == nullptr) {
    (void)CloseLineAndChip();
    return hal_interface::ErrorCode::kError;
  }

  gpiod_line_request_config parameters{};
  parameters.consumer = kGpiodConsumerName_;
  parameters.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
  parameters.flags = GetBiasFlags();

  if (::gpiod_line_request(line_, &parameters, 0) != 0) {
    (void)CloseLineAndChip();
    return hal_interface::ErrorCode::kError;
  }

  initialized_ = true;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode DigitalInput::Deinitialize() {
  return CloseLineAndChip();
}

bool DigitalInput::IsInitialized() {
  return initialized_;
}

hal_interface::ErrorCode DigitalInput::Read(bool& state, uint32_t timeout_ms) {
  (void)timeout_ms;

  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }

  int value = ::gpiod_line_get_value(line_);
  if (value < 0) {
    return hal_interface::ErrorCode::kError;
  }

  state = (value != 0);
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode DigitalInput::CloseLineAndChip() {
  if (line_ != nullptr) {
    ::gpiod_line_release(line_);
    line_ = nullptr;
  }

  if (chip_ != nullptr) {
    ::gpiod_chip_close(chip_);
    chip_ = nullptr;
  }

  initialized_ = false;
  return hal_interface::ErrorCode::kOk;
}

int DigitalInput::GetBiasFlags() const {
  switch (bias_) {
    case hal_interface::DigitalInput::LineBias::kNone:
      return GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE;
    case hal_interface::DigitalInput::LineBias::kPullDown:
      return GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN;
    case hal_interface::DigitalInput::LineBias::kPullUp:
      return GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;
    default:
      return GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE;
  }
}

}  // namespace sfw::hal_linux