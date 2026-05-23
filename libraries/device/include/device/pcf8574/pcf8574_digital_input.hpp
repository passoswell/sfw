// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef DEVICE_PCF8574_PCF8574_DIGITAL_INPUT_HPP
#define DEVICE_PCF8574_PCF8574_DIGITAL_INPUT_HPP

#include <cstdint>

#include "device/pcf8574/pcf8574.hpp"
#include "hal_interface/digital_input.hpp"

namespace sfw::device::pcf8574 {

/**
 * @brief PCF8574-backed implementation of hal_interface::DigitalInput.
 *
 * Reads one pin from a shared PCF8574 device object. During initialization,
 * the selected pin is released high so it can be sampled as an input.
 *
 * Include path: `#include "device/pcf8574/pcf8574_digital_input.hpp"`
 *
 * Typical usage:
 * 1. Construct DigitalInput with a reference to Pcf8574 and a pin index.
 * 2. Call Initialize() once before first Read().
 * 3. Call Read() to sample the selected pin.
 * 4. Call Deinitialize() when no longer needed.
 */
class DigitalInput final : public hal_interface::DigitalInput {
 public:
  /**
   * @brief Constructs a PCF8574-backed digital input.
   *
   * @param[in] expander Shared PCF8574 instance used for transfers.
   * @param[in] pin Pin index in range [0, 7].
   */
  DigitalInput(Pcf8574 &expander, uint8_t pin);

  DigitalInput(const DigitalInput &) = delete;
  DigitalInput &operator=(const DigitalInput &) = delete;
  DigitalInput(DigitalInput &&) = delete;
  DigitalInput &operator=(DigitalInput &&) = delete;

  /**
   * @brief Destructor
   */
  ~DigitalInput() override = default;

  /**
   * @brief Initializes the PCF8574 input pin.
   *
   * Calls Pcf8574::Initialize() and sets the configured pin high so it can be
   * sampled as an input.
   *
   * @retval ErrorCode::kOk Initialization succeeded.
   * @retval ErrorCode::kOutOfRange Pin index is invalid.
   * @retval ErrorCode::kTimeout I2C transfer timed out.
   * @retval ErrorCode::kError Initialization failed.
   */
  hal_interface::ErrorCode Initialize() override;

  /**
   * @brief Deinitializes this adapter instance.
   *
   * This only affects this wrapper state; the shared Pcf8574 object remains
   * available for other users.
   *
   * @retval ErrorCode::kOk Deinitialization succeeded.
   */
  hal_interface::ErrorCode Deinitialize() override;

  /**
   * @brief Returns whether this adapter was initialized successfully.
   *
   * @retval true This adapter is initialized.
   * @retval false This adapter is not initialized.
   */
  bool IsInitialized() override;

  /**
   * @brief Reads the configured input pin level.
   *
   * @param[out] state Receives the pin level.
   * @param[in] timeout_ms Maximum time to wait for the I2C transfer,
   * in milliseconds.
   * @retval ErrorCode::kOk Read succeeded.
   * @retval ErrorCode::kError Adapter is not initialized.
   * @retval ErrorCode::kOutOfRange Pin index is invalid.
   * @retval ErrorCode::kTimeout I2C transfer timed out.
   */
  hal_interface::ErrorCode Read(bool &state, uint32_t timeout_ms) override;

 private:
  Pcf8574 &expander_;
  uint8_t pin_;
  bool initialized_{false};

  static constexpr uint32_t kTimeoutMs{100};
};

}  // namespace sfw::device::pcf8574

#endif  // DEVICE_PCF8574_PCF8574_DIGITAL_INPUT_HPP
