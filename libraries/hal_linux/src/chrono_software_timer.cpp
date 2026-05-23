// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "hal_linux/software_timer/chrono_software_timer.hpp"

#include <chrono>
#include <cstdint>
#include <limits>
#include <thread>

namespace sfw::hal_linux {

ChronoSoftwareTimer::Duration ChronoSoftwareTimer::ToDuration(
    uint32_t value, hal_interface::TimeUnit unit) {
  switch (unit) {
    case hal_interface::TimeUnit::kMicroseconds:
      return std::chrono::microseconds{value};
    case hal_interface::TimeUnit::kMilliseconds:
      return std::chrono::milliseconds{value};
    case hal_interface::TimeUnit::kSeconds:
      return std::chrono::seconds{value};
    default:
      return Duration::zero();
  }
}

hal_interface::ErrorCode ChronoSoftwareTimer::ToUint32(
    Duration duration, hal_interface::TimeUnit unit, uint32_t& value) {
  int64_t converted = 0;

  switch (unit) {
    case hal_interface::TimeUnit::kMicroseconds:
      converted =
          std::chrono::duration_cast<std::chrono::microseconds>(duration)
              .count();
      break;
    case hal_interface::TimeUnit::kMilliseconds:
      converted =
          std::chrono::duration_cast<std::chrono::milliseconds>(duration)
              .count();
      break;
    case hal_interface::TimeUnit::kSeconds:
      converted =
          std::chrono::duration_cast<std::chrono::seconds>(duration).count();
      break;
    default:
      return hal_interface::ErrorCode::kInvalidArgument;
  }

  if ((converted < 0) || (converted > std::numeric_limits<uint32_t>::max())) {
    return hal_interface::ErrorCode::kOutOfRange;
  }

  value = static_cast<uint32_t>(converted);
  return hal_interface::ErrorCode::kOk;
}

ChronoSoftwareTimer::Duration ChronoSoftwareTimer::GetRawElapsedLocked(
    TimePoint now) const {
  if (!started_) {
    return Duration::zero();
  }

  if (stopped_) {
    return accumulated_active_;
  }

  return accumulated_active_ + (now - resume_time_);
}

ChronoSoftwareTimer::Duration ChronoSoftwareTimer::GetQueryElapsedLocked(
    TimePoint now) const {
  const Duration RawElapsed = GetRawElapsedLocked(now);
  if (auto_reload_ || (RawElapsed <= countdown_duration_)) {
    return RawElapsed;
  }

  return countdown_duration_;
}

bool ChronoSoftwareTimer::IsOneShotExpiredLocked(TimePoint now) const {
  if (!started_) {
    return true;
  }

  return GetRawElapsedLocked(now) >= countdown_duration_;
}

uint64_t ChronoSoftwareTimer::GetElapsedPeriodsLocked(TimePoint now) const {
  if (!started_ || (countdown_duration_ == Duration::zero())) {
    return 0;
  }

  const auto ElapsedCount = GetRawElapsedLocked(now).count();
  const auto PeriodCount = countdown_duration_.count();
  if ((ElapsedCount <= 0) || (PeriodCount <= 0)) {
    return 0;
  }

  return static_cast<uint64_t>(ElapsedCount / PeriodCount);
}

hal_interface::ErrorCode ChronoSoftwareTimer::Initialize() {
  const std::scoped_lock Lock{mutex_};
  initialized_ = true;
  started_ = false;
  stopped_ = false;
  power_on_time_ = Clock::now();
  accumulated_active_ = Duration::zero();
  countdown_duration_ = Duration::zero();
  reported_periods_ = 0;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode ChronoSoftwareTimer::Deinitialize() {
  const std::scoped_lock Lock{mutex_};
  initialized_ = false;
  started_ = false;
  stopped_ = false;
  auto_reload_ = false;
  accumulated_active_ = Duration::zero();
  countdown_duration_ = Duration::zero();
  reported_periods_ = 0;
  power_on_time_ = TimePoint{};
  resume_time_ = TimePoint{};
  return hal_interface::ErrorCode::kOk;
}

bool ChronoSoftwareTimer::IsInitialized() {
  const std::scoped_lock Lock{mutex_};
  return initialized_;
}

hal_interface::ErrorCode ChronoSoftwareTimer::Start(
    uint32_t time, hal_interface::TimeUnit unit) {
  const Duration RequestedDuration = ToDuration(time, unit);
  if (auto_reload_ && (RequestedDuration == Duration::zero())) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }

  const std::scoped_lock Lock{mutex_};
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }

  started_ = true;
  stopped_ = false;
  accumulated_active_ = Duration::zero();
  countdown_duration_ = RequestedDuration;
  resume_time_ = Clock::now();
  reported_periods_ = 0;
  return hal_interface::ErrorCode::kOk;
}

bool ChronoSoftwareTimer::HasExpired() {
  const std::scoped_lock Lock{mutex_};
  const TimePoint Now = Clock::now();
  if (!started_) {
    return true;
  }

  if (!auto_reload_) {
    return IsOneShotExpiredLocked(Now);
  }

  const uint64_t ElapsedPeriods = GetElapsedPeriodsLocked(Now);
  if (ElapsedPeriods > reported_periods_) {
    reported_periods_ = ElapsedPeriods;
    return true;
  }

  return false;
}

hal_interface::ErrorCode ChronoSoftwareTimer::Stop() {
  const std::scoped_lock Lock{mutex_};
  if (!initialized_ || !started_ || stopped_) {
    return hal_interface::ErrorCode::kError;
  }

  const TimePoint Now = Clock::now();
  if (!auto_reload_ && IsOneShotExpiredLocked(Now)) {
    return hal_interface::ErrorCode::kError;
  }

  accumulated_active_ = GetRawElapsedLocked(Now);
  stopped_ = true;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode ChronoSoftwareTimer::Resume() {
  const std::scoped_lock Lock{mutex_};
  if (!initialized_ || !started_ || !stopped_) {
    return hal_interface::ErrorCode::kError;
  }

  if (!auto_reload_ && (accumulated_active_ >= countdown_duration_)) {
    return hal_interface::ErrorCode::kOk;
  }

  stopped_ = false;
  resume_time_ = Clock::now();
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode ChronoSoftwareTimer::Delay(
    uint32_t value, hal_interface::TimeUnit unit) {
  {
    const std::scoped_lock Lock{mutex_};
    if (!initialized_) {
      return hal_interface::ErrorCode::kError;
    }
  }

  std::this_thread::sleep_for(ToDuration(value, unit));
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode ChronoSoftwareTimer::GetTimeSinceStart(
    uint32_t& value, hal_interface::TimeUnit unit) {
  const std::scoped_lock Lock{mutex_};
  if (!initialized_ || !started_) {
    return hal_interface::ErrorCode::kError;
  }

  return ToUint32(GetQueryElapsedLocked(Clock::now()), unit, value);
}

hal_interface::ErrorCode ChronoSoftwareTimer::GetTimeUntilExpiration(
    uint32_t& value, hal_interface::TimeUnit unit) {
  const std::scoped_lock Lock{mutex_};
  if (!initialized_ || !started_) {
    return hal_interface::ErrorCode::kError;
  }

  if (countdown_duration_ == Duration::zero()) {
    value = 0;
    return hal_interface::ErrorCode::kOk;
  }

  const Duration Elapsed = GetRawElapsedLocked(Clock::now());
  Duration remaining = Duration::zero();

  if (!auto_reload_) {
    if (Elapsed < countdown_duration_) {
      remaining = countdown_duration_ - Elapsed;
    }
  } else {
    const Duration Remainder = Elapsed % countdown_duration_;
    if (Remainder != Duration::zero()) {
      remaining = countdown_duration_ - Remainder;
    }
  }

  return ToUint32(remaining, unit, value);
}

hal_interface::ErrorCode ChronoSoftwareTimer::GetTimeSinceHwTimerPowerOn(
    uint32_t& value, hal_interface::TimeUnit unit) {
  const std::scoped_lock Lock{mutex_};
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }

  return ToUint32(Clock::now() - power_on_time_, unit, value);
}

hal_interface::ErrorCode ChronoSoftwareTimer::EnableAutoReload(bool enable) {
  const std::scoped_lock Lock{mutex_};
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }

  auto_reload_ = enable;
  reported_periods_ = 0;
  return hal_interface::ErrorCode::kOk;
}

}  // namespace sfw::hal_linux