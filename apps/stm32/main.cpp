// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "main.h"

#include "gpio.h"
#include "hal_stm32/dio/digital_output.hpp"

int main() {
  Stm32Initialize();

  sfw::hal_stm32::DigitalOutput led(GPIOC, 13, true);  // NOLINT
  (void)led.Initialize();

  while (true) {
    (void)led.Toggle(100U);  // NOLINT
    HAL_Delay(100);          // NOLINT
  }
}