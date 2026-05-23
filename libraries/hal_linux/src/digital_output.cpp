// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "hal_linux/dio/digital_output.hpp"

#include <gpiod.h>

#include <cstdint>
#include <string>
#include <utility>

namespace sfw::hal_linux {

namespace {

int GetDriveFlags(hal_interface::DigitalOutput::LineDrive line_drive) {
  switch (line_drive) {
    case hal_interface::DigitalOutput::LineDrive::kOpenDrain:
      return GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN;
    case hal_interface::DigitalOutput::LineDrive::kOpenSource:
      return GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE;
    case hal_interface::DigitalOutput::LineDrive::kPushPull:
    default:
      return 0;  // There is no definition for push-pull in libgpiod.
  }
}

int GetBiasFlags(hal_interface::DigitalOutput::LineBias line_bias) {
  switch (line_bias) {
    case hal_interface::DigitalOutput::LineBias::kDisable:
      return GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE;
    case hal_interface::DigitalOutput::LineBias::kPullDown:
      return GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN;
    case hal_interface::DigitalOutput::LineBias::kPullUp:
      return GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;
    default:
      return GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE;
  }
}

}  // namespace

DigitalOutput::DigitalOutput(std::string device, uint32_t line_offset,
                             bool initial_state, LineDrive line_drive,
                             LineBias line_bias)
  : device_{std::move(device)}
  , line_offset_{line_offset}
  , initial_state_{initial_state}
  , line_drive_{line_drive}
  , line_bias_{line_bias} {
}

DigitalOutput::~DigitalOutput() {
  (void)CloseLineAndChip();
}

hal_interface::ErrorCode DigitalOutput::Initialize() {
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
  parameters.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
  parameters.flags = GetDriveFlags(line_drive_) | GetBiasFlags(line_bias_);

  if (::gpiod_line_request(line_, &parameters, initial_state_ ? 1 : 0) != 0) {
    (void)CloseLineAndChip();
    return hal_interface::ErrorCode::kError;
  }
  state_ = initial_state_;
  initialized_ = true;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode DigitalOutput::Deinitialize() {
  if (initialized_) {
    (void)Write(initial_state_, 0);
  }
  return CloseLineAndChip();
}

bool DigitalOutput::IsInitialized() {
  return initialized_;
}

hal_interface::ErrorCode DigitalOutput::Write(bool state, uint32_t timeout_ms) {
  (void)timeout_ms;

  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }

  if (::gpiod_line_set_value(line_, state ? 1 : 0) != 0) {
    return hal_interface::ErrorCode::kError;
  }
  state_ = state;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode DigitalOutput::Toggle(uint32_t timeout_ms) {
  return Write(!state_, timeout_ms);
}

hal_interface::ErrorCode DigitalOutput::CloseLineAndChip() {
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

}  // namespace sfw::hal_linux
