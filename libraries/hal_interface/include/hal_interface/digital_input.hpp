// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_INTERFACE_DIGITAL_INPUT_HPP
#define HAL_INTERFACE_DIGITAL_INPUT_HPP

#include <cstdint>

#include "hal_interface/error_code.hpp"

namespace sfw::hal_interface {

/**
 * @brief Describes the type of edge event detected on a digital input pin.
 */
enum class DioEventType : uint8_t {
  kNoEdge = 0,   ///< No edge detected.
  kRisingEdge,   ///< A low-to-high transition was detected.
  kFallingEdge,  ///< A high-to-low transition was detected.
  kEitherEdge,   ///< Either a rising or falling transition was detected.
};

/**
 * @brief Abstract interface for digital input (GPIO) peripherals.
 *
 * All the methods on this interface are synchronous / blocking. This means
 * they only return if the operation is finished or if an error occurred.
 *
 * Typical usage:
 * 1. Call Initialize() once to configure the GPIO pin as an input.
 * 2. Call Read() to sample the current pin state.
 * 3. Call Deinitialize() when the peripheral is no longer needed.
 */
class DigitalInput {
 public:
  DigitalInput() = default;
  DigitalInput(const DigitalInput&) = default;
  DigitalInput& operator=(const DigitalInput&) = default;
  DigitalInput(DigitalInput&&) = default;
  DigitalInput& operator=(DigitalInput&&) = default;

  /**
   * @brief Destructor
   */
  virtual ~DigitalInput() = default;

  /**
   * @brief Initializes the digital input peripheral.
   *
   * Configures the underlying GPIO pin as an input and prepares the
   * event-capture mechanism. Must be called before any other method.
   *
   * @retval ErrorCode::kOk    Initialization succeeded.
   * @retval ErrorCode::kError Initialization failed due to a hardware fault.
   */
  virtual ErrorCode Initialize() = 0;

  /**
   * @brief Deinitializes the digital input peripheral.
   *
   * Releases resources allocated by Initialize(). After this call, Read()
   * must fail until Initialize() is called again.
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
   * @brief Reads the current logical state of the input pin.
   *
   * Samples the instantaneous pin level without affecting the event buffer.
   * May be called at any time, independent of event polling.
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
};

}  // namespace sfw::hal_interface

#endif /* HAL_INTERFACE_DIGITAL_INPUT_HPP */
