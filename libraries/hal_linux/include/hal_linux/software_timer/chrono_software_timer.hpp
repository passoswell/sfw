// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_LINUX_SOFTWARE_TIMER_CHRONO_SOFTWARE_TIMER_HPP
#define HAL_LINUX_SOFTWARE_TIMER_CHRONO_SOFTWARE_TIMER_HPP

#include <chrono>
#include <cstdint>
#include <mutex>

#include "hal_interface/software_timer.hpp"

namespace sfw::hal_linux {

/**
 * @brief Linux software timer backed by std::chrono::steady_clock.
 *
 * Uses std::chrono::steady_clock as a monotonic time source and
 * std::this_thread::sleep_for() for blocking delays. Countdown progress is
 * evaluated lazily on each API call, so no dedicated worker thread is needed.
 *
 * In auto-reload mode, HasExpired() returns true once for each elapsed period
 * since the previous HasExpired() call.
 *
 * Include path: `#include "hal_linux/software_timer/chrono_software_timer.hpp"`
 *
 * Typical usage:
 * 1. Construct ChronoSoftwareTimer.
 * 2. Call Initialize() to enable the monotonic time base.
 * 3. Call EnableAutoReload(false) for one-shot mode or
 *    EnableAutoReload(true) for periodic mode.
 * 4. Call Start() with the required duration and unit.
 * 5. Call HasExpired(), GetTimeSinceStart(), or GetTimeUntilExpiration() to
 *    observe timer progress.
 * 6. Call Stop() to pause the countdown and Resume() to continue it.
 * 7. Call Delay() for a blocking wait that does not affect the countdown.
 * 8. Call Deinitialize() when the timer backend is no longer needed.
 */
class ChronoSoftwareTimer final : public hal_interface::SoftwareTimer {
 public:
  /**
   * @brief Constructs a Linux software timer.
   */
  ChronoSoftwareTimer() = default;

  /**
   * @brief Destructor
   */
  ~ChronoSoftwareTimer() override = default;

  ChronoSoftwareTimer(const ChronoSoftwareTimer&) = delete;
  ChronoSoftwareTimer& operator=(const ChronoSoftwareTimer&) = delete;
  ChronoSoftwareTimer(ChronoSoftwareTimer&&) = delete;
  ChronoSoftwareTimer& operator=(ChronoSoftwareTimer&&) = delete;

  /**
   * @brief Initializes the steady-clock-backed timer backend.
   *
   * Captures the backend power-on timestamp used by
   * GetTimeSinceHwTimerPowerOn().
   *
   * @retval ErrorCode::kOk Backend initialized successfully.
   * @retval ErrorCode::kError Initialization failed.
   */
  hal_interface::ErrorCode Initialize() override;

  /**
   * @brief Deinitializes the timer backend and clears countdown state.
   *
   * After this call, a fresh Initialize() is required before starting another
   * countdown.
   *
   * @retval ErrorCode::kOk Backend deinitialized successfully.
   * @retval ErrorCode::kError Deinitialization failed.
   */
  hal_interface::ErrorCode Deinitialize() override;

  /**
   * @brief Returns whether the timer backend is initialized.
   *
   * @retval true The timer backend is ready.
   * @retval false The timer backend is not initialized.
   */
  bool IsInitialized() override;

  /**
   * @brief Starts a new countdown from the requested duration.
   *
   * Any previous countdown state is discarded.
   *
   * @param[in] time Countdown duration.
   * @param[in] unit Unit used by @p time.
   * @retval ErrorCode::kOk Countdown started successfully.
   * @retval ErrorCode::kInvalidArgument Auto-reload mode was requested with a
   * zero-length period.
   * @retval ErrorCode::kError The backend is not initialized.
   */
  hal_interface::ErrorCode Start(uint32_t time,
                                 hal_interface::TimeUnit unit) override;

  /**
   * @brief Reports whether the countdown expired.
   *
   * A timer that has not been started is treated as expired.
   *
   * @retval true The timer expired.
   * @retval false The timer has not yet expired.
   */
  bool HasExpired() override;

  /**
   * @brief Pauses a running countdown.
   *
   * The remaining time is preserved and can be resumed with Resume().
   *
   * @retval ErrorCode::kOk Countdown paused successfully.
   * @retval ErrorCode::kError The timer is not initialized or not running.
   */
  hal_interface::ErrorCode Stop() override;

  /**
   * @brief Resumes a countdown that was paused by Stop().
   *
   * If a one-shot timer has already expired while paused, the method leaves
   * it expired and returns success.
   *
   * @retval ErrorCode::kOk Countdown resumed successfully.
   * @retval ErrorCode::kError The timer is not initialized or not paused.
   */
  hal_interface::ErrorCode Resume() override;

  /**
   * @brief Blocks for the requested duration using std::this_thread::sleep_for.
   *
   * This delay is independent from any started countdown.
   *
   * @param[in] value Delay duration.
   * @param[in] unit Unit used by @p value.
   * @retval ErrorCode::kOk Delay completed successfully.
   * @retval ErrorCode::kError The backend is not initialized.
   */
  hal_interface::ErrorCode Delay(uint32_t value,
                                 hal_interface::TimeUnit unit) override;

  /**
   * @brief Returns the active countdown time since Start().
   *
   * Paused intervals are excluded. In one-shot mode, the returned time is
   * clamped to the configured duration after expiration.
   *
   * @param[out] value Receives the elapsed time.
   * @param[in] unit Unit used for @p value.
   * @retval ErrorCode::kOk Elapsed time retrieved successfully.
   * @retval ErrorCode::kOutOfRange The converted value does not fit in
   * uint32_t.
   * @retval ErrorCode::kError The timer is not initialized or not started.
   */
  hal_interface::ErrorCode GetTimeSinceStart(
      uint32_t& value, hal_interface::TimeUnit unit) override;

  /**
   * @brief Returns the remaining time until the next expiration.
   *
   * In auto-reload mode, this reports the time until the next period boundary.
   *
   * @param[out] value Receives the remaining time.
   * @param[in] unit Unit used for @p value.
   * @retval ErrorCode::kOk Remaining time retrieved successfully.
   * @retval ErrorCode::kOutOfRange The converted value does not fit in
   * uint32_t.
   * @retval ErrorCode::kError The timer is not initialized or not started.
   */
  hal_interface::ErrorCode GetTimeUntilExpiration(
      uint32_t& value, hal_interface::TimeUnit unit) override;

  /**
   * @brief Returns the elapsed backend uptime since Initialize().
   *
   * @param[out] value Receives the backend uptime.
   * @param[in] unit Unit used for @p value.
   * @retval ErrorCode::kOk Uptime retrieved successfully.
   * @retval ErrorCode::kOutOfRange The converted value does not fit in
   * uint32_t.
   * @retval ErrorCode::kError The backend is not initialized.
   */
  hal_interface::ErrorCode GetTimeSinceHwTimerPowerOn(
      uint32_t& value, hal_interface::TimeUnit unit) override;

  /**
   * @brief Enables or disables periodic auto-reload mode.
   *
   * The setting applies to the current countdown and to future Start() calls.
   *
   * @param[in] enable True for periodic mode, false for one-shot mode.
   * @retval ErrorCode::kOk Setting applied successfully.
   * @retval ErrorCode::kError The backend is not initialized.
   */
  hal_interface::ErrorCode EnableAutoReload(bool enable) override;

 private:
  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;
  using TimePoint = Clock::time_point;

  [[nodiscard]] static Duration ToDuration(uint32_t value,
                                           hal_interface::TimeUnit unit);
  [[nodiscard]] static hal_interface::ErrorCode ToUint32(
      Duration duration, hal_interface::TimeUnit unit, uint32_t& value);
  [[nodiscard]] Duration GetRawElapsedLocked(TimePoint now) const;
  [[nodiscard]] Duration GetQueryElapsedLocked(TimePoint now) const;
  [[nodiscard]] bool IsOneShotExpiredLocked(TimePoint now) const;
  [[nodiscard]] uint64_t GetElapsedPeriodsLocked(TimePoint now) const;

  std::mutex mutex_;
  bool initialized_{false};
  bool started_{false};
  bool stopped_{false};
  bool auto_reload_{false};
  TimePoint power_on_time_{};
  TimePoint resume_time_{};
  Duration accumulated_active_{Duration::zero()};
  Duration countdown_duration_{Duration::zero()};
  uint64_t reported_periods_{0};
};

}  // namespace sfw::hal_linux

#endif  // HAL_LINUX_SOFTWARE_TIMER_CHRONO_SOFTWARE_TIMER_HPP