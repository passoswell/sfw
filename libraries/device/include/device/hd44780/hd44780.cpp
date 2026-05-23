// Copyright (c) 2026 sfw contributors. All rights reserved.

#include "hd44780.hpp"

namespace sfw::device::hd44780 {

Hd44780::Hd44780(interface::Hd44780Bus &bus,
                 hal_interface::SoftwareTimer &timer, Geometry geometry)
  : bus_(bus)
  , timer_(timer)
  , geometry_(geometry)
  , rows_(ResolveRows(geometry))
  , columns_(ResolveColumns(geometry)) {
}

Hd44780::~Hd44780() {
  (void)Deinitialize();
}

hal_interface::ErrorCode Hd44780::Initialize() {
  if (initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  hal_interface::ErrorCode result = InitializeDependencies();
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = DelayMs(kDelayPowerOnMs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  uint8_t function_set = kCommandFunctionSet;
  if (bus_.GetWidth() == interface::Hd44780Bus::Width::k8Bits) {
    function_set = static_cast<uint8_t>(function_set | kFlagFunction8Bits);
  }
  if (rows_ > 1U) {
    function_set = static_cast<uint8_t>(function_set | kFlagFunction2Lines);
  }

  result = bus_.WriteCommand(function_set);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = InitializeDefaultDisplayState();
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  initialized_ = true;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode Hd44780::InitializeDependencies() {
  if (!timer_.IsInitialized()) {
    const hal_interface::ErrorCode TimerResult = timer_.Initialize();
    if (TimerResult != hal_interface::ErrorCode::kOk) {
      return TimerResult;
    }
  }
  return bus_.Initialize();
}

hal_interface::ErrorCode Hd44780::InitializeDefaultDisplayState() {
  display_enabled_ = false;
  cursor_enabled_ = false;
  blink_enabled_ = false;
  entry_increment_ = true;
  entry_shift_ = false;
  current_row_ = 0U;
  current_column_ = 0U;

  hal_interface::ErrorCode result = bus_.WriteCommand(kCommandDisplayControl);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = bus_.WriteCommand(kCommandClear);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = DelayUs(kDelayClearHomeUs);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  const auto EntryModeCommand =
      static_cast<uint8_t>(kCommandEntryModeSet | kFlagEntryIncrement);
  result = bus_.WriteCommand(EntryModeCommand);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  display_enabled_ = true;
  const auto DisplayCommand =
      static_cast<uint8_t>(kCommandDisplayControl | kFlagDisplayOn);
  return bus_.WriteCommand(DisplayCommand);
}

hal_interface::ErrorCode Hd44780::Deinitialize() {
  if (!initialized_) {
    return hal_interface::ErrorCode::kOk;
  }

  auto result = Clear();
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = SetDisplayEnabled(false);
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  result = bus_.Deinitialize();
  if (result != hal_interface::ErrorCode::kOk) {
    return result;
  }

  initialized_ = false;
  return hal_interface::ErrorCode::kOk;
}

bool Hd44780::IsInitialized() const {
  return initialized_;
}

hal_interface::ErrorCode Hd44780::Clear() {
  const hal_interface::ErrorCode Result = WriteCommand(kCommandClear);
  if (Result != hal_interface::ErrorCode::kOk) {
    return Result;
  }

  current_row_ = 0U;
  current_column_ = 0U;
  return DelayUs(kDelayClearHomeUs);
}

hal_interface::ErrorCode Hd44780::Home() {
  const hal_interface::ErrorCode Result = WriteCommand(kCommandHome);
  if (Result != hal_interface::ErrorCode::kOk) {
    return Result;
  }

  current_row_ = 0U;
  current_column_ = 0U;
  return DelayUs(kDelayClearHomeUs);
}

hal_interface::ErrorCode Hd44780::SetCursor(uint8_t row, uint8_t column) {
  if (row >= rows_ || column >= columns_) {
    return hal_interface::ErrorCode::kOutOfRange;
  }

  const auto Address =
      static_cast<uint8_t>(GetRowOffset(row) + static_cast<uint8_t>(column));
  const hal_interface::ErrorCode Result =
      WriteCommand(static_cast<uint8_t>(kCommandSetDdramAddress | Address));
  if (Result != hal_interface::ErrorCode::kOk) {
    return Result;
  }

  current_row_ = row;
  current_column_ = column;
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode Hd44780::WriteChar(char character) {
  if (character == '\n') {
    const auto NextRow = static_cast<uint8_t>((current_row_ + 1U) % rows_);
    const hal_interface::ErrorCode CursorResult =
        SetCursor(NextRow, current_column_);
    if (CursorResult != hal_interface::ErrorCode::kOk) {
      return CursorResult;
    }
    return hal_interface::ErrorCode::kOk;
  }
  if (character == '\r') {
    const hal_interface::ErrorCode CursorResult = SetCursor(current_row_, 0U);
    if (CursorResult != hal_interface::ErrorCode::kOk) {
      return CursorResult;
    }
    return hal_interface::ErrorCode::kOk;
  }

  // Check if the character is printable
  if (character < kFirstPrintableChar || character > kLastPrintableChar) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }

  const hal_interface::ErrorCode Result =
      WriteData(static_cast<uint8_t>(character));
  if (Result != hal_interface::ErrorCode::kOk) {
    return Result;
  }

  if (entry_increment_) {
    if (current_column_ + 1U < columns_) {
      ++current_column_;
    }
  } else {
    if (current_column_ > 0U) {
      --current_column_;
    }
  }

  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode Hd44780::WriteText(const char *text) {
  if (text == nullptr) {
    return hal_interface::ErrorCode::kInvalidArgument;
  }

  return WriteText(std::string_view(text));
}

hal_interface::ErrorCode Hd44780::WriteText(std::string_view text) {
  for (const char Character : text) {
    const hal_interface::ErrorCode WriteResult = WriteChar(Character);
    if (WriteResult != hal_interface::ErrorCode::kOk) {
      return WriteResult;
    }
  }
  return hal_interface::ErrorCode::kOk;
}

hal_interface::ErrorCode Hd44780::SetDisplayEnabled(bool enabled) {
  const bool Previous = display_enabled_;
  display_enabled_ = enabled;
  const hal_interface::ErrorCode Result = UpdateDisplayControl();
  if (Result != hal_interface::ErrorCode::kOk) {
    display_enabled_ = Previous;
  }
  return Result;
}

hal_interface::ErrorCode Hd44780::SetCursorEnabled(bool enabled) {
  const bool Previous = cursor_enabled_;
  cursor_enabled_ = enabled;
  const hal_interface::ErrorCode Result = UpdateDisplayControl();
  if (Result != hal_interface::ErrorCode::kOk) {
    cursor_enabled_ = Previous;
  }
  return Result;
}

hal_interface::ErrorCode Hd44780::SetBlinkEnabled(bool enabled) {
  const bool Previous = blink_enabled_;
  blink_enabled_ = enabled;
  const hal_interface::ErrorCode Result = UpdateDisplayControl();
  if (Result != hal_interface::ErrorCode::kOk) {
    blink_enabled_ = Previous;
  }
  return Result;
}

hal_interface::ErrorCode Hd44780::SetEntryMode(bool increment,
                                               bool shift_display) {
  entry_increment_ = increment;
  entry_shift_ = shift_display;

  uint8_t command = kCommandEntryModeSet;
  if (entry_increment_) {
    command = static_cast<uint8_t>(command | kFlagEntryIncrement);
  }
  if (entry_shift_) {
    command = static_cast<uint8_t>(command | kFlagEntryShift);
  }

  return WriteCommand(command);
}

hal_interface::ErrorCode Hd44780::ShiftDisplay(bool right) {
  auto command = static_cast<uint8_t>(kCommandCursorShift | kFlagShiftDisplay);
  if (right) {
    command = static_cast<uint8_t>(command | kFlagShiftRight);
  }

  return WriteCommand(command);
}

hal_interface::ErrorCode Hd44780::CreateCustomChar(
    uint8_t slot, std::span<const uint8_t, kCgramRowsPerSlot> bitmap) {
  if (!initialized_ || bitmap.size() != kCgramRowsPerSlot) {
    return hal_interface::ErrorCode::kError;
  }
  if (slot >= kCgramSlotCount) {
    return hal_interface::ErrorCode::kError;
  }

  const auto Address = static_cast<uint8_t>((slot * kCgramRowsPerSlot) & 0x3FU);
  const auto Command = static_cast<uint8_t>(kCommandSetCgramAddress | Address);
  if (WriteCommand(Command) != hal_interface::ErrorCode::kOk) {
    return hal_interface::ErrorCode::kError;
  }

  for (uint8_t index = 0U; index < kCgramRowsPerSlot; ++index) {
    const auto RowData = static_cast<uint8_t>(bitmap[index] & kCgramDataMask);
    if (WriteData(RowData) != hal_interface::ErrorCode::kOk) {
      return hal_interface::ErrorCode::kError;
    }
  }

  return SetCursor(current_row_, current_column_);
}

hal_interface::ErrorCode Hd44780::WriteCustomChar(uint8_t slot) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  if (slot >= kCgramSlotCount) {
    return hal_interface::ErrorCode::kError;
  }

  const auto GlyphCode = static_cast<uint8_t>(slot & kCgramSlotMask);
  if (WriteData(GlyphCode) != hal_interface::ErrorCode::kOk) {
    return hal_interface::ErrorCode::kError;
  }

  if (entry_increment_) {
    if (current_column_ + 1U < columns_) {
      ++current_column_;
    }
  } else {
    if (current_column_ > 0U) {
      --current_column_;
    }
  }

  return hal_interface::ErrorCode::kOk;
}

uint8_t Hd44780::GetRowCount() const {
  return rows_;
}

uint8_t Hd44780::GetColumnCount() const {
  return columns_;
}

hal_interface::ErrorCode Hd44780::DelayUs(uint32_t microseconds) {
  return timer_.Delay(microseconds, hal_interface::TimeUnit::kMicroseconds);
}

hal_interface::ErrorCode Hd44780::DelayMs(uint32_t milliseconds) {
  return timer_.Delay(milliseconds, hal_interface::TimeUnit::kMilliseconds);
}

hal_interface::ErrorCode Hd44780::WriteCommand(uint8_t value) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  return bus_.WriteCommand(value);
}

hal_interface::ErrorCode Hd44780::WriteData(uint8_t value) {
  if (!initialized_) {
    return hal_interface::ErrorCode::kError;
  }
  return bus_.WriteData(value);
}

hal_interface::ErrorCode Hd44780::UpdateDisplayControl() {
  uint8_t command = kCommandDisplayControl;
  if (display_enabled_) {
    command = static_cast<uint8_t>(command | kFlagDisplayOn);
  }
  if (cursor_enabled_) {
    command = static_cast<uint8_t>(command | kFlagCursorOn);
  }
  if (blink_enabled_) {
    command = static_cast<uint8_t>(command | kFlagBlinkOn);
  }
  return WriteCommand(command);
}

uint8_t Hd44780::GetRowOffset(uint8_t row) const {
  if (geometry_ == Geometry::k20x4) {
    switch (row) {
      case 0U:
        return kRowOffsetLine0;
      case 1U:
        return kRowOffsetLine1;
      case 2U:
        return kRowOffsetLine2;
      case 3U:
        return kRowOffsetLine3;
      default:
        return kRowOffsetLine0;
    }
  }

  if (rows_ > 1U) {
    return (row == 0U) ? kRowOffsetLine0 : kRowOffsetLine1;
  }
  return kRowOffsetLine0;
}

uint8_t Hd44780::ResolveRows(Geometry geometry) {
  switch (geometry) {
    case Geometry::k8x1:
    case Geometry::k16x1:
      return 1U;
    case Geometry::k16x2:
    case Geometry::k20x2:
    case Geometry::k40x2:
      return 2U;
    case Geometry::k20x4:
      return 4U;
    default:
      return 2U;
  }
}

uint8_t Hd44780::ResolveColumns(Geometry geometry) {
  switch (geometry) {
    case Geometry::k8x1:
      return kColumns8;
    case Geometry::k16x1:
    case Geometry::k16x2:
      return kColumns16;
    case Geometry::k20x2:
    case Geometry::k20x4:
      return kColumns20;
    case Geometry::k40x2:
      return kColumns40;
    default:
      return kColumns16;
  }
}

}  // namespace sfw::device::hd44780