// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_INTERFACE_SOFTWARE_TIMER_HPP
#define HAL_INTERFACE_SOFTWARE_TIMER_HPP

#include <cstdint>

#include "hal_interface/error_code.hpp"

namespace sfw::hal_interface {

/**
 * @brief Specifies the time unit used in SoftwareTimer methods.
 */
enum class TimeUnit : uint8_t {
  kMicroseconds = 0,  ///< Time expressed in microseconds.
  kMilliseconds,      ///< Time expressed in milliseconds.
  kSeconds,           ///< Time expressed in seconds.
};

/**
 * @brief Abstract interface for software-managed countdown and elapsed-time
 * timers.
 *
 * A SoftwareTimer wraps a hardware time source and provides one-shot or
 * auto-reloading countdown functionality, elapsed-time queries, and a
 * blocking delay primitive.
 * One hardware timer can be shared among multiple SoftwareTimer instances.
 * All duration arguments use the TimeUnit enum so that callers can work in the
 * most natural unit for the application.
 * All the methods on this interface are synchronous / blocking. This means they
 * only return if the operation is finished or if an error occurred.
 *
 * Typical usage:
 * 1. Call Initialize() once to prepare timer resources.
 * 2. Call EnableAutoReload(true) for periodic mode or EnableAutoReload(false)
 * for one-shot mode.
 * 3. Call Start() with a duration and time unit to begin the countdown.
 * 4. Call HasExpired() to poll for expiration, or GetTimeUntilExpiration() to
 * check remaining time.
 * 5. Call Stop() to pause and Resume() to continue from where it was paused.
 * 6. Call Delay() for simple blocking delays without affecting the countdown
 * state.
 * 7. Call Deinitialize() when the peripheral is no longer needed.
 */
class SoftwareTimer {
 public:
  SoftwareTimer() = default;
  SoftwareTimer(const SoftwareTimer&) = default;
  SoftwareTimer& operator=(const SoftwareTimer&) = default;
  SoftwareTimer(SoftwareTimer&&) = default;
  SoftwareTimer& operator=(SoftwareTimer&&) = default;

  /**
   * @brief Destructor
   */
  virtual ~SoftwareTimer() = default;

  /**
   * @brief Initializes the software timer backend.
   *
   * Must be called before any timer control/query method.
   *
   * @retval ErrorCode::kOk Timer initialized successfully.
   * @retval ErrorCode::kError Initialization failed.
   */
  virtual ErrorCode Initialize() = 0;

  /**
   * @brief Deinitializes the software timer backend.
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
   * @brief Returns whether the backend has been successfully initialized.
   *
   * @retval true  Initialize() has been called and succeeded.
   * @retval false The backend is not yet initialized.
   */
  virtual bool IsInitialized() = 0;

  /**
   * @brief Starts the timer with the specified countdown duration.
   *
   * Records the start time and sets the expiration point to @p time @p unit
   * in the future. If the timer was already running it is restarted from
   * scratch. If auto-reload is enabled (see EnableAutoReload()), the timer
   * restarts automatically when it expires.
   *
   * @param[in] time Duration of the countdown.
   * @param[in] unit Time unit in which @p time is expressed.
   * @retval ErrorCode::kOk    The timer was started successfully.
   * @retval ErrorCode::kError The underlying hardware timer is unavailable.
   */
  virtual ErrorCode Start(uint32_t time, TimeUnit unit) = 0;

  /**
   * @brief Returns whether the countdown has expired.
   *
   * A timer that has not been started is considered as expired.
   *
   * @retval true  The timer reached its expiration point.
   * @retval false The timer is still counting down or has not been started.
   */
  virtual bool HasExpired() = 0;

  /**
   * @brief Stops the timer without resetting the elapsed-time state.
   *
   * After Stop(), HasExpired() continues to reflect the state at the moment
   * of the stop. Call Resume() to continue counting, or Start() to restart.
   *
   * @retval ErrorCode::kOk    The timer was stopped successfully.
   * @retval ErrorCode::kError The timer was not running.
   */
  virtual ErrorCode Stop() = 0;

  /**
   * @brief Resumes a stopped timer from where it left off.
   *
   * Continues counting from the remaining time at the point Stop() was called.
   * Has no effect if the timer has already expired.
   *
   * @retval ErrorCode::kOk    The timer was resumed successfully.
   * @retval ErrorCode::kError The timer was not in a stopped state.
   */
  virtual ErrorCode Resume() = 0;

  /**
   * @brief Blocks the calling context for the specified duration.
   *
   * This is a busy-wait or sleep primitive. It does not interact with the
   * timer's countdown state.
   *
   * @param[in] value Duration to delay.
   * @param[in] unit  Time unit in which @p value is expressed.
   * @retval ErrorCode::kOk    The delay completed successfully.
   * @retval ErrorCode::kError The underlying hardware timer is unavailable.
   */
  virtual ErrorCode Delay(uint32_t value, TimeUnit unit) = 0;

  /**
   * @brief Returns the elapsed time since the soft timer was last started.
   *
   * @param[out] value Receives the elapsed time in @p unit units.
   * @param[in]  unit  Time unit for the returned value.
   * @retval ErrorCode::kOk    The elapsed time was retrieved successfully.
   * @retval ErrorCode::kError The timer has not been started.
   */
  virtual ErrorCode GetTimeSinceStart(uint32_t& value, TimeUnit unit) = 0;

  /**
   * @brief Returns the remaining time until the soft timer expires.
   *
   * Returns zero if the soft timer has already expired.
   *
   * @param[out] value Receives the remaining time in @p unit units.
   * @param[in]  unit  Time unit for the returned value.
   * @retval ErrorCode::kOk    The remaining time was retrieved successfully.
   * @retval ErrorCode::kError The timer has not been started.
   */
  virtual ErrorCode GetTimeUntilExpiration(uint32_t& value, TimeUnit unit) = 0;

  /**
   * @brief Returns the time elapsed since the underlying hardware timer was
   * powered on.
   *
   * Provides a free-running monotonic clock independent of any countdown
   * state. Useful for timestamping events across multiple soft timers'
   * start/stop cycles.
   *
   * @param[out] value Receives the time since power-on in @p unit units.
   * @param[in]  unit  Time unit for the returned value.
   * @retval ErrorCode::kOk    The uptime was retrieved successfully.
   * @retval ErrorCode::kError The hardware timer is unavailable.
   */
  virtual ErrorCode GetTimeSinceHwTimerPowerOn(uint32_t& value,
                                               TimeUnit unit) = 0;

  /**
   * @brief Enables or disables automatic reload of the countdown on expiration.
   *
   * When enabled, the timer restarts with the same duration each time it
   * expires, producing a periodic tick. When disabled, the timer stops after
   * a single expiration.
   *
   * @param[in] enable True to enable auto-reload (periodic mode), false for
   * one-shot mode.
   * @retval ErrorCode::kOk    The auto-reload setting was applied successfully.
   * @retval ErrorCode::kError The setting could not be applied.
   */
  virtual ErrorCode EnableAutoReload(bool enable) = 0;
};

}  // namespace sfw::hal_interface

#endif /* HAL_INTERFACE_SOFTWARE_TIMER_HPP */
