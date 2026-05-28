// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_INTERFACE_DIGITAL_OUTPUT_HPP
#define HAL_INTERFACE_DIGITAL_OUTPUT_HPP

#include <cstdint>

#include "hal_interface/error_code.hpp"

namespace sfw::hal_interface {

/**
 * @brief Abstract interface for digital output (DIO) peripherals.
 *
 * Provides methods to initialize a DIO pin configured as a push-pull or
 * open-drain output and to control its logical level.
 * All the methods on this interface are synchronous / blocking. This means
 * they only return if the operation is finished or if an error occurred.
 * Note that not all platforms may support all line drive and line bias
 * configurations.
 *
 * Typical usage:
 * 1. Call Initialize() once to configure the DIO pin as an output.
 * 2. Call Write() to set the output level, or Toggle() to invert it.
 * 3. Repeat step 2 as needed to control the output level.
 * 4. Call Deinitialize() when the peripheral is no longer needed.
 */
class DigitalOutput {
 public:
  /**
   * @brief Output drive mode for the requested DIO line.
   */
  enum class LineDrive : uint8_t {
    kPushPull = 0,
    kOpenDrain,
    kOpenSource,
  };

  /**
   * @brief Internal bias mode for the requested DIO line.
   */
  enum class LineBias : uint8_t {
    kNone = 0,
    kPullDown,
    kPullUp,
  };

  DigitalOutput() = default;
  DigitalOutput(const DigitalOutput&) = default;
  DigitalOutput& operator=(const DigitalOutput&) = default;
  DigitalOutput(DigitalOutput&&) = default;
  DigitalOutput& operator=(DigitalOutput&&) = default;

  /**
   * @brief Destructor
   */
  virtual ~DigitalOutput() = default;

  /**
   * @brief Initializes the digital output peripheral.
   *
   * Configures the underlying DIO pin as an output. The initial output level
   * after initialization is implementation-defined and may be specified on the
   * constructor of the concrete implementation. Must be called before Write()
   * or Toggle().
   * If any configuration option informed through the implementation's
   * constructor is not supported, this method must return an error.
   *
   * @retval ErrorCode::kOk    Initialization succeeded.
   * @retval ErrorCode::kError Initialization failed due to a hardware fault.
   */
  virtual ErrorCode Initialize() = 0;

  /**
   * @brief Deinitializes the digital output peripheral.
   *
   * Releases resources allocated by Initialize(). After this call, Write()
   * and Toggle() must fail until Initialize() is called again.
   * Registers are deinitialized and peripherals can be put in low power or
   * off mode.
   *
   * @retval ErrorCode::kOk Peripheral deinitialized successfully.
   * @retval ErrorCode::kError Deinitialization failed.
   */
  virtual ErrorCode Deinitialize() = 0;

  /**
   * @brief Returns whether the peripheral has been successfully initialized.
   *
   * @retval true  Initialize() has been called and succeeded.
   * @retval false The peripheral is not yet initialized.
   */
  virtual bool IsInitialized() = 0;

  /**
   * @brief Drives the output pin to the specified logical level.
   *
   * Sets the output to the specified level. This method may be called
   * immediately after Write(); subsequent calls override the previous level.
   *
   * @param[in] state      Desired output level: true for high, false for low.
   * @param[in] timeout_ms Maximum time to wait for the write to take effect, in
   * milliseconds.
   * @retval ErrorCode::kOk      The output level was set successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode Write(bool state, uint32_t timeout_ms) = 0;

  /**
   * @brief Inverts the current output level of the pin.
   *
   * Equivalent to reading the current driven level and writing its complement,
   * but may be implemented atomically in hardware for faster toggling.
   *
   * @param[in] timeout_ms Maximum time to wait for the toggle to take effect,
   * in milliseconds.
   * @retval ErrorCode::kOk      The output level was toggled successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode Toggle(uint32_t timeout_ms) = 0;
};

}  // namespace sfw::hal_interface

#endif  // HAL_INTERFACE_DIGITAL_OUTPUT_HPP