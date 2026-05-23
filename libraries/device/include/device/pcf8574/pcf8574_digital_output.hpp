// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef DEVICE_PCF8574_PCF8574_DIGITAL_OUTPUT_HPP
#define DEVICE_PCF8574_PCF8574_DIGITAL_OUTPUT_HPP

#include <cstdint>

#include "device/pcf8574/pcf8574.hpp"
#include "hal_interface/digital_output.hpp"

namespace sfw::device::pcf8574 {

/**
 * @brief PCF8574-backed implementation of hal_interface::DigitalOutput.
 *
 * Controls one pin from a shared PCF8574 device object.
 *
 * Include path: `#include "device/pcf8574/pcf8574_digital_output.hpp"`
 *
 * Typical usage:
 * 1. Construct DigitalOutput with a reference to Pcf8574 and a pin index.
 * 2. Call Initialize() once before first Write() or Toggle().
 * 3. Call Write() to set output level.
 * 4. Call Toggle() to invert output level.
 * 5. Call Deinitialize() when no longer needed.
 */
class DigitalOutput final : public hal_interface::DigitalOutput {
 public:
  /**
   * @brief Constructs a PCF8574-backed digital output.
   *
   * @param[in] expander Shared PCF8574 instance used for transfers.
   * @param[in] pin Pin index in range [0, 7].
   * @param[in] initial_state Initial pin level set during Initialize().
   */
  DigitalOutput(Pcf8574 &expander, uint8_t pin, bool initial_state);

  DigitalOutput(const DigitalOutput &) = delete;
  DigitalOutput &operator=(const DigitalOutput &) = delete;
  DigitalOutput(DigitalOutput &&) = delete;
  DigitalOutput &operator=(DigitalOutput &&) = delete;

  /**
   * @brief Destructor
   */
  ~DigitalOutput() override = default;

  /**
   * @brief Initializes the PCF8574 output pin.
   *
   * Calls Pcf8574::Initialize() and writes the configured initial output
   * level to the selected pin.
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
   * This method also attempts to reset the pin to the initial state.
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
   * @brief Writes the configured output pin level.
   *
   * @param[in] state Desired output level.
   * @param[in] timeout_ms Maximum time to wait for the I2C transfer,
   * in milliseconds.
   * @retval ErrorCode::kOk Write succeeded.
   * @retval ErrorCode::kError Adapter is not initialized.
   * @retval ErrorCode::kOutOfRange Pin index is invalid.
   * @retval ErrorCode::kTimeout I2C transfer timed out.
   */
  hal_interface::ErrorCode Write(bool state, uint32_t timeout_ms) override;

  /**
   * @brief Toggles the configured output pin.
   *
   * The toggle is performed by writing the complement of the current output
   * state.
   *
   * @param[in] timeout_ms Maximum time to wait for each I2C transfer,
   * in milliseconds.
   * @retval ErrorCode::kOk Toggle succeeded.
   * @retval ErrorCode::kError Adapter is not initialized.
   * @retval ErrorCode::kOutOfRange Pin index is invalid.
   * @retval ErrorCode::kTimeout I2C transfer timed out.
   */
  hal_interface::ErrorCode Toggle(uint32_t timeout_ms) override;

 private:
  Pcf8574 &expander_;
  uint8_t pin_;
  bool initial_state_;
  bool state_{false};
  bool initialized_{false};

  static constexpr uint32_t kTimeoutMs{100};
};

}  // namespace sfw::device::pcf8574

#endif  // DEVICE_PCF8574_PCF8574_DIGITAL_OUTPUT_HPP
