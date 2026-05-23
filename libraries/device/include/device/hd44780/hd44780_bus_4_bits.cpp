// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "device/hd44780/hd44780_bus_4_bits.hpp"

namespace sfw::device::hd44780 {

Hd44780Bus4Bits::Hd44780Bus4Bits(hal_interface::DigitalOutput &rs_pin,
                                 hal_interface::DigitalOutput &en_pin,
                                 hal_interface::DigitalOutput &data4_pin,
                                 hal_interface::DigitalOutput &data5_pin,
                                 hal_interface::DigitalOutput &data6_pin,
                                 hal_interface::DigitalOutput &data7_pin,
                                 hal_interface::SoftwareTimer &timer,
                                 hal_interface::DigitalOutput *wr_pin)
  : rs_(rs_pin)
  , en_(en_pin)
  , d4_(data4_pin)
  , d5_(data5_pin)
  , d6_(data6_pin)
  , d7_(data7_pin)
  , timer_(timer)
  , wr_(wr_pin) {
}

hal_interface::ErrorCode Hd44780Bus4Bits::Initialize() {
  if (initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  hal_interface::ErrorCode first_error = hal_interface::ErrorCode::kOk;
  auto update_first_error = [&first_error](hal_interface::ErrorCode error) {
    if (first_error == hal_interface::ErrorCode::kOk
        && error != hal_interface::ErrorCode::kOk) {
      first_error = error;
    }
  };

  // Running all initialization steps. If one fails, Initialize() fails.
  if (wr_ != nullptr) {
    update_first_error(InitializePin(*wr_, false));
  }
  update_first_error(InitializePin(rs_, false));

  update_first_error(InitializePin(en_, false));
  update_first_error(InitializePin(d4_, false));
  update_first_error(InitializePin(d5_, false));
  update_first_error(InitializePin(d6_, false));
  update_first_error(InitializePin(d7_, false));

  update_first_error(timer_.Initialize());

  update_first_error(DelayUs(kDelayPowerOnUs));

  update_first_error(WriteInitializationNibble(0x03U));
  update_first_error(DelayUs(kDelayInitStep1Us));
  update_first_error(WriteInitializationNibble(0x03U));
  update_first_error(DelayUs(kDelayInitStep2Us));
  update_first_error(WriteInitializationNibble(0x03U));
  update_first_error(DelayUs(kDelayInitStep3Us));
  update_first_error(WriteInitializationNibble(0x02U));

  if (first_error == hal_interface::ErrorCode::kOk) {
    initialized_ = true;
  }
  return first_error;
}

hal_interface::ErrorCode Hd44780Bus4Bits::Deinitialize() {
  if (!initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  hal_interface::ErrorCode first_error = hal_interface::ErrorCode::kOk;
  auto update_first_error = [&first_error](hal_interface::ErrorCode error) {
    if (first_error == hal_interface::ErrorCode::kOk
        && error != hal_interface::ErrorCode::kOk) {
      first_error = error;
    }
  };

  // Running all initialization steps. If one fails, Deinitialize() fails.
  update_first_error(rs_.Write(false, kPinTimeoutMs));
  update_first_error(en_.Write(false, kPinTimeoutMs));
  update_first_error(WriteNibble(0x00U));

  if (wr_ != nullptr) {
    update_first_error(wr_->Deinitialize());
  }
  update_first_error(rs_.Deinitialize());
  update_first_error(en_.Deinitialize());
  update_first_error(d4_.Deinitialize());
  update_first_error(d5_.Deinitialize());
  update_first_error(d6_.Deinitialize());
  update_first_error(d7_.Deinitialize());
  update_first_error(timer_.Deinitialize());

  if (first_error == hal_interface::ErrorCode::kOk) {
    initialized_ = false;
  }
  return first_error;
}

bool Hd44780Bus4Bits::IsInitialized() {
  return initialized_;
}

hal_interface::ErrorCode Hd44780Bus4Bits::WriteCommand(uint8_t value) {
  return WriteByte(false, value);
}

hal_interface::ErrorCode Hd44780Bus4Bits::WriteData(uint8_t value) {
  return WriteByte(true, value);
}

interface::Hd44780Bus::Width Hd44780Bus4Bits::GetWidth() const {
  return interface::Hd44780Bus::Width::k4Bits;
}

hal_interface::ErrorCode Hd44780Bus4Bits::DelayUs(uint32_t microseconds) {
  return timer_.Delay(microseconds, hal_interface::TimeUnit::kMicroseconds);
}

hal_interface::ErrorCode Hd44780Bus4Bits::InitializePin(
    hal_interface::DigitalOutput &pin, bool state) {
  auto result = pin.Initialize();
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }
  result = pin.Write(state, kPinTimeoutMs);
  return result;
}

hal_interface::ErrorCode Hd44780Bus4Bits::PulseEnable() {
  auto result = en_.Write(true, kPinTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = DelayUs(kDelayEnablePulseUs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = en_.Write(false, kPinTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  return DelayUs(kDelayEnablePulseUs);
}

hal_interface::ErrorCode Hd44780Bus4Bits::WriteByte(bool rs_state,
                                                    uint8_t value) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }

  hal_interface::ErrorCode result = rs_.Write(rs_state, kPinTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = WriteNibble(static_cast<uint8_t>((value >> 4U) & kNibbleMask));
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = PulseEnable();
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = WriteNibble(static_cast<uint8_t>(value & kNibbleMask));
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = PulseEnable();
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  return DelayUs(kDelayWriteCycleUs);
}

hal_interface::ErrorCode Hd44780Bus4Bits::WriteNibble(uint8_t nibble) {
  hal_interface::ErrorCode result =
      d4_.Write((nibble & kBit0Mask) != 0U, kPinTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }
  result = d5_.Write((nibble & kBit1Mask) != 0U, kPinTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }
  result = d6_.Write((nibble & kBit2Mask) != 0U, kPinTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }
  return d7_.Write((nibble & kBit3Mask) != 0U, kPinTimeoutMs);
}

hal_interface::ErrorCode Hd44780Bus4Bits::WriteInitializationNibble(
    uint8_t nibble) {
  hal_interface::ErrorCode result = rs_.Write(false, kPinTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = WriteNibble(nibble);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  return PulseEnable();
}

}  // namespace sfw::device::hd44780