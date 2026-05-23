// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_INTERFACE_TIMER_HPP
#define HAL_INTERFACE_TIMER_HPP

#include <cstdint>
#include <functional>

#include "hal_interface/error_code.hpp"

namespace sfw::hal_interface {

/**
 * @brief Callback type invoked on timer overflow or update events.
 *
 * @param[in] user_arg Opaque pointer supplied when the callback was registered
 *                     via SetCallback(). May be nullptr if not used.
 */
using TimerCallback = std::function<void(void* user_arg)>;

/**
 * @brief Abstract interface for a hardware timer/counter peripheral.
 *
 * Provides direct control over a hardware timer including start, stop, reset,
 * prescaler, and auto-reload configuration, as well as an optional
 * interrupt-driven callback mechanism for periodic event handling.
 * All the methods on this interface are synchronous / blocking. This means
 * they only return if the operation is finished or if an error occurred.
 *
 * Typical usage:
 * 1. Call Initialize() once to prepare timer resources.
 * 2. Call SetRegisters() to configure the prescaler and auto-reload values.
 * 3. Call SetCallback() to register an interrupt handler if overflow
 *    notifications are needed.
 * 4. Call Start() to begin counting, Reset() to clear the counter, and Stop()
 *    to halt it.
 * 5. Call GetCounter() to read the current counter value, or GetFrequencyHz()
 *    to determine the tick rate.
 * 6. Call Deinitialize() when the peripheral is no longer needed.
 */
class Timer {
 public:
  Timer() = default;
  Timer(const Timer&) = default;
  Timer& operator=(const Timer&) = default;
  Timer(Timer&&) = default;
  Timer& operator=(Timer&&) = default;

  /**
   * @brief Destructor
   */
  virtual ~Timer() = default;

  /**
   * @brief Initializes the timer peripheral.
   *
   * Must be called before any control or query method.
   *
   * @retval ErrorCode::kOk Timer initialized successfully.
   * @retval ErrorCode::kError Initialization failed.
   */
  virtual ErrorCode Initialize() = 0;

  /**
   * @brief Deinitializes the timer peripheral.
   *
   * Releases resources allocated by Initialize(). After this call, methods
   * except Initialize() and IsInitialized() must fail.
   * Registers are deinitialized and peripherals can be put in low power or
   * off mode.
   *
   * @retval ErrorCode::kOk Timer deinitialized successfully.
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
   * @brief Starts the hardware timer counter.
   *
   * Enables the timer peripheral and begins incrementing the counter. If a
   * callback has been registered it can be invoked on each overflow or
   * update event from within an interrupt context. Does not reset the counter,
   * call Reset() first if a fresh count is required.
   *
   * @param[in] timeout_ms Maximum time to wait for timer start, in
   * milliseconds.
   * @retval ErrorCode::kOk      The timer started successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   * @retval ErrorCode::kError The timer could not be started.
   */
  virtual ErrorCode Start(uint32_t timeout_ms) = 0;

  /**
   * @brief Stops the hardware timer counter.
   *
   * Halts the counter without resetting it or removing any registered callback.
   * The counter value is preserved and may be read via GetCounter() after
   * stopping.
   *
   * @param[in] timeout_ms Maximum time to wait for timer stop, in
   * milliseconds.
   * @retval ErrorCode::kOk      The timer stopped successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   * @retval ErrorCode::kError The timer could not be stopped.
   */
  virtual ErrorCode Stop(uint32_t timeout_ms) = 0;

  /**
   * @brief Resets the hardware counter to zero.
   *
   * Sets the counter register to 0 without affecting the running state. May
   * be called while the timer is running or stopped.
   *
   * @param[in] timeout_ms Maximum time to wait for reset completion, in
   * milliseconds.
   * @retval ErrorCode::kOk      The counter was reset successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   * @retval ErrorCode::kError The counter could not be reset.
   */
  virtual ErrorCode Reset(uint32_t timeout_ms) = 0;

  /**
   * @brief Configures the timer prescaler and auto-reload register.
   *
   * The bit width of a timer is platform dependent and may be informed either
   * through the constructor of the concrete implementation or via a vendor's
   * helper function. It limits the maximum values for @p prescaler and
   * @p auto_reload.
   * Changes take effect immediately.
   *
   * @param[in] prescaler   Prescaler value.
   * @param[in] auto_reload Auto-reload register value.
   * @retval ErrorCode::kOk    Registers were set successfully.
   * @retval ErrorCode::kError The configuration could not be applied.
   */
  virtual ErrorCode SetRegisters(uint32_t prescaler, uint32_t auto_reload) = 0;

  /**
   * @brief Returns the current value of the hardware counter register.
   *
   * @retval Current counter value in the range [0, GetMaxCountValuePossible()].
   */
  virtual uint32_t GetCounter() = 0;

  /**
   * @brief Returns the maximum counter value that is configured for the timer.
   *
   * The maximum value possible configured for the counter will probably
   * be between 0 and the value configured for the auto-reload register when
   * calling SetRegisters().
   *
   * @retval Maximum representable counter value.
   */
  virtual uint32_t GetMaxCountValuePossible() = 0;

  /**
   * @brief Returns the timer input clock frequency in Hz after prescaling.
   *
   * This is the rate at which the counter increments. Useful for converting
   * counter values to time units.
   *
   * @retval Timer counter frequency in Hz.
   */
  virtual uint32_t GetFrequencyHz() = 0;

  /**
   * @brief Registers a callback to be invoked on timer overflow or update
   * events.
   *
   * The callback is called from an interrupt context, so it must be kept
   * short and must not call blocking functions. Pass nullptr to deregister
   * any previously registered callback.
   *
   * @param[in] callback Function to invoke on each timer overflow/update event.
   * @retval ErrorCode::kOk    The callback was registered successfully.
   * @retval ErrorCode::kError The callback could not be registered.
   */
  virtual ErrorCode SetCallback(TimerCallback callback) = 0;
};

}  // namespace sfw::hal_interface

#endif  // HAL_INTERFACE_TIMER_HPP