// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "device/hd44780/hd44780_bus_8_bits.hpp"

namespace sfw::device::hd44780 {

Hd44780Bus8Bits::Hd44780Bus8Bits(hal_interface::DigitalOutput &rs_pin,
                                 hal_interface::DigitalOutput &en_pin,
                                 hal_interface::DigitalOutput &data0_pin,
                                 hal_interface::DigitalOutput &data1_pin,
                                 hal_interface::DigitalOutput &data2_pin,
                                 hal_interface::DigitalOutput &data3_pin,
                                 hal_interface::DigitalOutput &data4_pin,
                                 hal_interface::DigitalOutput &data5_pin,
                                 hal_interface::DigitalOutput &data6_pin,
                                 hal_interface::DigitalOutput &data7_pin,
                                 hal_interface::SoftwareTimer &timer,
                                 hal_interface::DigitalOutput *wr_pin)
  : rs_(rs_pin)
  , en_(en_pin)
  , d0_(data0_pin)
  , d1_(data1_pin)
  , d2_(data2_pin)
  , d3_(data3_pin)
  , d4_(data4_pin)
  , d5_(data5_pin)
  , d6_(data6_pin)
  , d7_(data7_pin)
  , timer_(timer)
  , wr_(wr_pin) {
}

hal_interface::ErrorCode Hd44780Bus8Bits::Initialize() {
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

  if (wr_ != nullptr) {
    update_first_error(InitializePin(*wr_, false));
  }
  update_first_error(InitializePin(rs_, false));
  update_first_error(InitializePin(en_, false));
  update_first_error(InitializePin(d0_, false));
  update_first_error(InitializePin(d1_, false));
  update_first_error(InitializePin(d2_, false));
  update_first_error(InitializePin(d3_, false));
  update_first_error(InitializePin(d4_, false));
  update_first_error(InitializePin(d5_, false));
  update_first_error(InitializePin(d6_, false));
  update_first_error(InitializePin(d7_, false));
  update_first_error(timer_.Initialize());

  update_first_error(DelayUs(kDelayPowerOnUs));

  update_first_error(WriteCommand(kCommandInit8Bits));
  update_first_error(DelayUs(kDelayFunctionSetStepUs));
  update_first_error(WriteCommand(kCommandInit8Bits));
  update_first_error(DelayUs(kDelayFunctionSetRepeatUs));
  update_first_error(WriteCommand(kCommandInit8Bits));
  update_first_error(DelayUs(kDelayFunctionSetRepeatUs));

  if (first_error == hal_interface::ErrorCode::kOk) {
    initialized_ = true;
  }
  return first_error;
}

hal_interface::ErrorCode Hd44780Bus8Bits::Deinitialize() {
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

  update_first_error(rs_.Write(false, kPinTimeoutMs));
  update_first_error(en_.Write(false, kPinTimeoutMs));
  update_first_error(SetDataPins(0x00U));
  if (wr_ != nullptr) {
    update_first_error(wr_->Deinitialize());
  }
  update_first_error(rs_.Deinitialize());
  update_first_error(en_.Deinitialize());
  update_first_error(d0_.Deinitialize());
  update_first_error(d1_.Deinitialize());
  update_first_error(d2_.Deinitialize());
  update_first_error(d3_.Deinitialize());
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

bool Hd44780Bus8Bits::IsInitialized() {
  return initialized_;
}

hal_interface::ErrorCode Hd44780Bus8Bits::WriteCommand(uint8_t value) {
  return WriteByte(false, value);
}

hal_interface::ErrorCode Hd44780Bus8Bits::WriteData(uint8_t value) {
  return WriteByte(true, value);
}

interface::Hd44780Bus::Width Hd44780Bus8Bits::GetWidth() const {
  return interface::Hd44780Bus::Width::k8Bits;
}

hal_interface::ErrorCode Hd44780Bus8Bits::DelayUs(uint32_t microseconds) {
  return timer_.Delay(microseconds, hal_interface::TimeUnit::kMicroseconds);
}

hal_interface::ErrorCode Hd44780Bus8Bits::InitializePin(
    hal_interface::DigitalOutput &pin, bool state) {
  auto result = pin.Initialize();
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }
  result = pin.Write(state, kPinTimeoutMs);
  return result;
}

hal_interface::ErrorCode Hd44780Bus8Bits::PulseEnable() {
  hal_interface::ErrorCode result = en_.Write(true, kPinTimeoutMs);
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

hal_interface::ErrorCode Hd44780Bus8Bits::WriteByte(bool rs_state,
                                                    uint8_t value) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }

  hal_interface::ErrorCode result = rs_.Write(rs_state, kPinTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = SetDataPins(value);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = PulseEnable();
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  return DelayUs(kDelayWriteCycleUs);
}

hal_interface::ErrorCode Hd44780Bus8Bits::SetDataPins(uint8_t value) {
  hal_interface::ErrorCode result =
      d0_.Write((value & kBit0Mask) != 0U, kPinTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }
  result = d1_.Write((value & kBit1Mask) != 0U, kPinTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }
  result = d2_.Write((value & kBit2Mask) != 0U, kPinTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }
  result = d3_.Write((value & kBit3Mask) != 0U, kPinTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }
  result = d4_.Write((value & kBit4Mask) != 0U, kPinTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }
  result = d5_.Write((value & kBit5Mask) != 0U, kPinTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }
  result = d6_.Write((value & kBit6Mask) != 0U, kPinTimeoutMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }
  return d7_.Write((value & kBit7Mask) != 0U, kPinTimeoutMs);
}

}  // namespace sfw::device::hd44780