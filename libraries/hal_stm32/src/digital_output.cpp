// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "hal_stm32/dio/digital_output.hpp"

namespace sfw::hal_stm32 {

namespace {

GPIO_PinState ToPinState(bool state) {
  return state ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

}  // namespace

DigitalOutput::DigitalOutput(GPIO_TypeDef* gpio_port, uint16_t gpio_pin,
                             bool initial_state)
  : gpio_port_{gpio_port}
  , gpio_pin_{static_cast<uint16_t>(1U << gpio_pin)}
  , initial_state_{initial_state} {
}

hal_interface::ErrorCode DigitalOutput::Initialize() {
  if (initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  if ((gpio_port_ == nullptr) || (gpio_pin_ == 0U)) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }

  HAL_GPIO_WritePin(gpio_port_, gpio_pin_, ToPinState(initial_state_));
  state_ = initial_state_;
  initialized_ = true;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode DigitalOutput::Deinitialize() {
  if (initialized_) {
    HAL_GPIO_WritePin(gpio_port_, gpio_pin_, ToPinState(initial_state_));
  }

  initialized_ = false;
  state_ = initial_state_;
  return hal_interface::ErrorCode::kOk;
}

bool DigitalOutput::IsInitialized() {
  return initialized_;
}

hal_interface::ErrorCode DigitalOutput::Write(bool state, uint32_t timeout_ms) {
  (void)timeout_ms;

  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }

  HAL_GPIO_WritePin(gpio_port_, gpio_pin_, ToPinState(state));
  state_ = state;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode DigitalOutput::Toggle(uint32_t timeout_ms) {
  (void)timeout_ms;

  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }

  HAL_GPIO_TogglePin(gpio_port_, gpio_pin_);
  state_ = !state_;
  return hal_interface::ErrorCode::kOk;
}

}  // namespace sfw::hal_stm32