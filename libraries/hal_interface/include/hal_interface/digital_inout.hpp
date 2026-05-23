// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_INTERFACE_DIGITAL_INOUT_HPP
#define HAL_INTERFACE_DIGITAL_INOUT_HPP

#include <cstdint>

#include "hal_interface/error_code.hpp"

namespace sfw::hal_interface {

/**
 * @brief Abstract interface for bidirectional digital I/O (GPIO) peripherals.
 *
 * Allows a single pin to be dynamically reconfigured as either an input or an
 * output at runtime. Typical use cases include open-drain buses and
 * half-duplex protocols that share a single line for both directions.
 * All the methods on this interface are synchronous / blocking. This means
 * they only return if the operation is finished or if an error occurred.
 *
 * Typical usage:
 * 1. Call Initialize() once to configure the GPIO pin.
 * 2. Call SetAsInput() or SetAsOutput() to set the pin direction.
 * 3. Call Read() to sample the pin state.
 * 4. Call Write() or Toggle() to change the output level (if configured as
 * output).
 * 5. Repeat steps 2-4 as needed, dynamically reconfiguring the pin direction as
 * required.
 * 6. Call Deinitialize() when the peripheral is no longer needed.
 */
class DigitalInOut {
 public:
  DigitalInOut() = default;
  DigitalInOut(const DigitalInOut&) = default;
  DigitalInOut& operator=(const DigitalInOut&) = default;
  DigitalInOut(DigitalInOut&&) = default;
  DigitalInOut& operator=(DigitalInOut&&) = default;

  /**
   * @brief Destructor
   */
  virtual ~DigitalInOut() = default;

  /**
   * @brief Initializes the digital I/O peripheral.
   *
   * Configures the underlying GPIO pin and prepares it for use. Must be
   * called before any other method. The initial direction is
   * implementation-defined, this means it can be set, for instance, through a
   * constructor parameter in the implementation class; call SetAsInput() or
   * SetAsOutput() to set it explicitly.
   *
   * @retval ErrorCode::kOk    Initialization succeeded.
   * @retval ErrorCode::kError Initialization failed due to a hardware fault.
   */
  virtual ErrorCode Initialize() = 0;

  /**
   * @brief Deinitializes the digital I/O peripheral.
   *
   * Releases resources allocated by Initialize(). After this call, all
   * operations except Initialize() and IsInitialized() must fail.
   * Registers are deinitialized and peripherals can be put in low power or
   * off mode.
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
   * @brief Reconfigures the pin as a digital input.
   *
   * After this call, Read() may be used to sample the pin. Calls to Write()
   * and Toggle() must return an error while the pin is configured as an input.
   *
   * @param[in] timeout_ms Maximum time to wait for the reconfiguration, in
   * milliseconds.
   * @retval ErrorCode::kOk      Pin successfully reconfigured as input.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode SetAsInput(uint32_t timeout_ms) = 0;

  /**
   * @brief Reconfigures the pin as a digital output with the given initial
   * state.
   *
   * The @p state is driven onto the pin atomically with the direction change
   * to prevent glitches. After this call, Write() and Toggle() may be used.
   *
   * @param[in] state      Initial output level: true for high, false for low.
   * @param[in] timeout_ms Maximum time to wait for the reconfiguration, in
   * milliseconds.
   * @retval ErrorCode::kOk      Pin successfully reconfigured as output.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode SetAsOutput(bool state, uint32_t timeout_ms) = 0;

  /**
   * @brief Reads the current logical state of the pin.
   *
   * May be called when the pin is configured as either an input or an output.
   * When configured as an output, the returned value reflects the driven level.
   *
   * @param[out] state     Receives the current pin level: true for high, false
   * for low.
   * @param[in]  timeout_ms Maximum time to wait for a valid reading, in
   * milliseconds.
   * @retval ErrorCode::kOk      The state was read successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode Read(bool& state, uint32_t timeout_ms) = 0;

  /**
   * @brief Drives the output pin to the specified logical level.
   *
   * The pin must have been configured as an output via SetAsOutput() before
   * calling this method.
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
   * Equivalent to reading the current state and writing its complement, but
   * may be implemented atomically in hardware. The pin must be configured as
   * an output before calling this method.
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

#endif  // HAL_INTERFACE_DIGITAL_INOUT_HPP