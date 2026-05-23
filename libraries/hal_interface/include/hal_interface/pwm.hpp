// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_INTERFACE_PWM_HPP
#define HAL_INTERFACE_PWM_HPP

#include <cstdint>
#include <span>

#include "hal_interface/error_code.hpp"

namespace sfw::hal_interface {

/**
 * @brief Abstract interface for PWM (Pulse-Width Modulation) output
 * peripherals.
 *
 * Provides methods to set duty cycles in either normalized floating-point form
 * or raw timer-count form, and to start or stop the PWM signal.
 * All the methods on this interface are synchronous / blocking. This means
 * they only return if the operation is finished or if an error occurred.
 *
 * Typical usage:
 * 1. Call Initialize() once to configure the PWM peripheral.
 * 2. Call Start() once to enable PWM signal generation.
 * 3. Call Write() for normalized duty cycles (0.0–1.0) or WriteRaw() for raw
 * timer counts.
 * 4. Repeat step 3 to adjust duty cycles as needed.
 * 5. Call Stop() when PWM output is no longer needed.
 * 6. Call Deinitialize() when the peripheral is no longer needed.
 */
class PWM {
 public:
  PWM() = default;
  PWM(const PWM&) = default;
  PWM& operator=(const PWM&) = default;
  PWM(PWM&&) = default;
  PWM& operator=(PWM&&) = default;

  /**
   * @brief Destructor
   */
  virtual ~PWM() = default;

  /**
   * @brief Initializes the PWM peripheral.
   *
   * Configures timer channels and output GPIOs required for PWM generation.
   * Must be called before Start(), Write(), or WriteRaw().
   * The important parameters can be passed through the constructor of the
   * concrete implementation, if needed.
   *
   * @retval ErrorCode::kOk Peripheral initialized successfully.
   * @retval ErrorCode::kError Initialization failed.
   */
  virtual ErrorCode Initialize() = 0;

  /**
   * @brief Deinitializes the PWM peripheral.
   *
   * Releases resources allocated by Initialize(). After this call, Start(),
   * Write(), and WriteRaw() must fail until Initialize() is called again.
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
   * @brief Sets the duty cycle as normalized floats.
   *
   * Each element of @p buffer represents the duty cycle for one channel, where
   * 0.0 corresponds to always-off and 1.0 corresponds to always-on. Values
   * outside [0.0, 1.0] are clamped. The channel, as well as the time this
   * method must wait between elements of @p buffer, depend on the
   * implementation and should be specified in its constructor, if needed.
   * To achieve its end, the implementation is free to use a software loop or a
   * DMA transfer, for instance. However, this method must return only when the
   * operation is completed or in case of failure.
   * This method may be called before Start() to set the initial duty cycle, or
   * while the PWM is running for dynamic updates.
   *
   * @param[in] buffer     Source span whose elements set the duty cycle per
   * channel.
   * @param[in] timeout_ms Maximum time to wait for the update to take effect,
   * in milliseconds.
   * @retval ErrorCode::kOk      Duty cycles were set successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode Write(std::span<float> buffer, uint32_t timeout_ms) = 0;

  /**
   * @brief Sets the duty cycle as raw timer counts.
   *
   * Each element of @p buffer is written in sequence directly to the
   * corresponding implementation's timer register. The channel, as well as the
   * time this method must wait between elements of @p buffer, depend on the
   * implementation and should be specified in its constructor if needed.
   * To achieve its end, the implementation is free to use a software loop or a
   * DMA transfer, for instance. However, this method must return only when the
   * operation is completed or in case of failure.
   * Values must not exceed the configured auto-reload register value, or its
   * equivalent in the implementation.
   * This method may be called before Start() to set the initial raw count, or
   * while the PWM is running for dynamic updates.
   *
   * @param[in] buffer     Source span whose elements set the raw compare value
   * per channel.
   * @param[in] timeout_ms Maximum time to wait for the update to take effect,
   * in milliseconds.
   * @retval ErrorCode::kOk      Raw duty cycles were set successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode WriteRaw(std::span<uint32_t> buffer,
                             uint32_t timeout_ms) = 0;

  /**
   * @brief Starts PWM signal generation.
   *
   * Enables the underlying timer and begins toggling the output according to
   * the configured duty cycle. Must be called before the PWM output is active.
   * Prior calls to Write() or WriteRaw() set the initial duty cycle.
   *
   * @param[in] timeout_ms Maximum time to wait for the peripheral to start, in
   * milliseconds.
   * @retval ErrorCode::kOk      PWM output started successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode Start(uint32_t timeout_ms) = 0;

  /**
   * @brief Stops PWM signal generation.
   *
   * Disables the underlying timer output. The output pin level after stopping
   * is implementation-defined (typically held low). Call Start() to resume
   * PWM generation from the previously configured duty cycle.
   *
   * @param[in] timeout_ms Maximum time to wait for the peripheral to stop, in
   * milliseconds.
   * @retval ErrorCode::kOk      PWM output stopped successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode Stop(uint32_t timeout_ms) = 0;
};

}  // namespace sfw::hal_interface

#endif  // HAL_INTERFACE_PWM_HPP
