// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef DEVICE_HD44780_HD44780_HPP
#define DEVICE_HD44780_HD44780_HPP

#include <cstdint>
#include <span>
#include <string_view>

#include "device/hd44780/interface/hd44780_bus_interface.hpp"
#include "hal_interface/error_code.hpp"
#include "hal_interface/software_timer.hpp"

namespace sfw::device::hd44780 {

/**
 * @brief Supported HD44780-compatible display geometries.
 */
enum class Geometry : uint8_t {
  k8x1 = 0,
  k16x1,
  k16x2,
  k20x2,
  k20x4,
  k40x2,
};

static constexpr uint8_t kHd44780CgramRowsPerSlot{8U};

/**
 * @brief High-level driver for HD44780-compatible character displays.
 *
 * This class exposes commonly used HD44780 display features through a
 * write-only bus abstraction. It supports the most common geometries and does
 * not rely on busy-flag reads; required delays are enforced with a software
 * timer.
 * All methods are blocking and return only after the operation is complete.
 * No timeouts are configurable for the overall methods operation. However, a
 * reasonable default timeout value is used with internal call to functions of
 * the injected bus and timer backends, which can be adjusted by modifying the
 * kPinTimeoutMs constant in the bus implementations.
 *
 * Include path: `#include "device/hd44780/hd44780.hpp"`
 *
 * Typical usage:
 * 1. Construct the class with a concrete Hd44780Bus implementation,
 * a software timer and display geometry.
 * 2. Call Initialize() once.
 * 3. Use Clear(), SetCursor(), WriteChar() and WriteText() to update content.
 * 4. Optionally control cursor, blink, display visibility and shifts.
 * 5. Call Deinitialize() when the instance is no longer needed.
 */
class Hd44780 final {
 public:
  /**
   * @brief Constructs an HD44780 display driver.
   *
   * Stores references to the injected bus and timer backends and resolves
   * geometry-dependent limits used by cursor positioning and text writes.
   * This constructor does not access hardware.
   *
   * @param[in] bus Electrical bus abstraction used for all transfers.
   * @param[in] timer Software timer used for controller execution delays.
   * @param[in] geometry Display geometry.
   */
  Hd44780(interface::Hd44780Bus &bus, hal_interface::SoftwareTimer &timer,
          Geometry geometry);

  Hd44780(const Hd44780 &) = delete;
  Hd44780 &operator=(const Hd44780 &) = delete;
  Hd44780(Hd44780 &&) = delete;
  Hd44780 &operator=(Hd44780 &&) = delete;

  /**
   * @brief Destructor
   */
  ~Hd44780();

  /**
   * @brief Initializes the display and applies default configuration.
   *
   * This method configures bus width, line mode, entry mode and display
   * control state following the HD44780 startup flow without read operations.
   * The visible result is a known clean screen with cursor at home and display
   * enabled.
   *
   * @retval hal_interface::ErrorCode::kOk Initialization completed.
   * @retval hal_interface::ErrorCode::kError Bus or timer operation failed.
   */
  hal_interface::ErrorCode Initialize();

  /**
   * @brief Deinitializes this driver instance.
   *
   * The method marks this object as unavailable and deinitializes the injected
   * bus abstraction. After success, display operations through this instance
   * must not be used until Initialize() is called again.
   *
   * @retval hal_interface::ErrorCode::kOk Deinitialization completed.
   * @retval hal_interface::ErrorCode::kError Bus deinitialization failed.
   */
  hal_interface::ErrorCode Deinitialize();

  /**
   * @brief Returns whether this driver instance is initialized.
   *
   * Use this state query to gate application-level display writes.
   *
   * @retval true Driver is initialized.
   * @retval false Driver is not initialized.
   */
  [[nodiscard]] bool IsInitialized() const;

  /**
   * @brief Clears the display and moves cursor to home.
   *
   * Sends the HD44780 clear instruction. This clears visible DDRAM content,
   * resets the cursor to row 0, column 0 and waits the required execution
   * time.
   *
   * @retval hal_interface::ErrorCode::kOk Command accepted.
   * @retval hal_interface::ErrorCode::kError Driver not initialized or bus
   * write failed.
   */
  hal_interface::ErrorCode Clear();

  /**
   * @brief Returns cursor to home address.
   *
   * Sends the return-home instruction. Display content is preserved, but the
   * active write position is restored to the start of the display.
   *
   * @retval hal_interface::ErrorCode::kOk Command accepted.
   * @retval hal_interface::ErrorCode::kError Driver not initialized or bus
   * write failed.
   */
  hal_interface::ErrorCode Home();

  /**
   * @brief Sets cursor to a given row/column.
   *
   * Converts the logical row/column pair to a controller DDRAM address based
   * on the configured geometry and updates the next write position.
   *
   * @param[in] row Target row index.
   * @param[in] column Target column index.
   * @retval hal_interface::ErrorCode::kOk Cursor updated.
   * @retval hal_interface::ErrorCode::kOutOfRange Row or column is invalid.
   * @retval hal_interface::ErrorCode::kError Driver not initialized or bus
   * write failed.
   */
  hal_interface::ErrorCode SetCursor(uint8_t row, uint8_t column);

  /**
   * @brief Writes one character at the current cursor position.
   *
   * Sends one data byte to the display DDRAM. Cursor progression follows the
   * currently configured entry mode.
   *
   * @param[in] character Character byte to send to DDRAM.
   * @retval hal_interface::ErrorCode::kOk Character written.
   * @retval hal_interface::ErrorCode::kError Driver not initialized or bus
   * write failed.
   */
  hal_interface::ErrorCode WriteChar(char character);

  /**
   * @brief Writes text from a null-terminated C string.
   *
   * Writes characters sequentially from @p text at the current cursor
   * position until the null terminator is reached.
   *
   * @param[in] text Null-terminated text pointer.
   * @retval hal_interface::ErrorCode::kOk Text written.
   * @retval hal_interface::ErrorCode::kInvalidArgument text is null.
   * @retval hal_interface::ErrorCode::kError Driver not initialized or bus
   * write failed.
   */
  hal_interface::ErrorCode WriteText(const char *text);

  /**
   * @brief Writes text from a string view.
   *
   * Writes all characters from @p text sequentially starting at the current
   * cursor location.
   *
   * @param[in] text Text view to send.
   * @retval hal_interface::ErrorCode::kOk Text written.
   * @retval hal_interface::ErrorCode::kError Driver not initialized or bus
   * write failed.
   */
  hal_interface::ErrorCode WriteText(std::string_view text);

  /**
   * @brief Enables or disables display visibility.
   *
   * Updates the display ON/OFF control bit. Disabling visibility does not
   * erase DDRAM content.
   *
   * @param[in] enabled True to turn display on, false to turn display off.
   * @retval hal_interface::ErrorCode::kOk Command accepted.
   * @retval hal_interface::ErrorCode::kError Driver not initialized or bus
   * write failed.
   */
  hal_interface::ErrorCode SetDisplayEnabled(bool enabled);

  /**
   * @brief Enables or disables the underline cursor.
   *
   * Updates the cursor-visibility control bit while preserving current text
   * content and cursor address.
   *
   * @param[in] enabled True to show cursor, false to hide cursor.
   * @retval hal_interface::ErrorCode::kOk Command accepted.
   * @retval hal_interface::ErrorCode::kError Driver not initialized or bus
   * write failed.
   */
  hal_interface::ErrorCode SetCursorEnabled(bool enabled);

  /**
   * @brief Enables or disables cursor blinking.
   *
   * Updates the blink control bit of the display mode register.
   *
   * @param[in] enabled True to enable blink, false to disable blink.
   * @retval hal_interface::ErrorCode::kOk Command accepted.
   * @retval hal_interface::ErrorCode::kError Driver not initialized or bus
   * write failed.
   */
  hal_interface::ErrorCode SetBlinkEnabled(bool enabled);

  /**
   * @brief Configures text entry direction and display shift-on-write.
   *
   * Controls how the controller updates cursor/display after each data write:
   * cursor increment/decrement and optional automatic display shift.
   *
   * @param[in] increment True for left-to-right cursor increment, false for
   * right-to-left.
   * @param[in] shift_display True to shift display on each data write, false to
   * leave display unchanged.
   * @retval hal_interface::ErrorCode::kOk Command accepted.
   * @retval hal_interface::ErrorCode::kError Driver not initialized or bus
   * write failed.
   */
  hal_interface::ErrorCode SetEntryMode(bool increment, bool shift_display);

  /**
   * @brief Performs a display shift without changing DDRAM.
   *
   * Shifts the visible window left or right while keeping stored character
   * data unchanged.
   *
   * @param[in] right True to shift right, false to shift left.
   * @retval hal_interface::ErrorCode::kOk Command accepted.
   * @retval hal_interface::ErrorCode::kError Driver not initialized or bus
   * write failed.
   */
  hal_interface::ErrorCode ShiftDisplay(bool right);

  /**
   * @brief Creates or overwrites one CGRAM custom character slot.
   *
   * Writes 8 row bytes into the selected CGRAM slot. Calling this method
   * again with the same slot replaces the previously stored glyph.
   *
   * Example:
   * @code
   * const uint8_t smiley[8] = {
   *   0x00U, 0x0AU, 0x00U, 0x00U,
   *   0x11U, 0x0EU, 0x00U, 0x00U
   * };
   * (void) lcd.CreateCustomChar(0, smiley);
   * @endcode
   *
   * @param[in] slot CGRAM slot index in range [0, 7].
   * @param[in] bitmap Span of 8 row bytes for the custom glyph.
   * @retval hal_interface::ErrorCode::kOk Custom character was stored
   * successfully.
   * @retval hal_interface::ErrorCode::kError Driver not initialized, invalid
   * slot or bus write failed.
   */
  hal_interface::ErrorCode CreateCustomChar(
      uint8_t slot, std::span<const uint8_t, kHd44780CgramRowsPerSlot> bitmap);

  /**
   * @brief Writes one previously created custom character to the display.
   *
   * Writes the custom character code associated with the selected CGRAM slot
   * at the current cursor position.
   *
   * @param[in] slot CGRAM slot index in range [0, 7].
   * @retval hal_interface::ErrorCode::kOk Character code was written
   * successfully.
   * @retval hal_interface::ErrorCode::kError Driver not initialized, invalid
   * slot or bus write failed.
   */
  hal_interface::ErrorCode WriteCustomChar(uint8_t slot);

  /**
   * @brief Returns configured display row count.
   *
   * This value comes from the selected Geometry and is used for bounds checks
   * in cursor positioning.
   *
   * @retval uint8_t Row count for selected geometry.
   */
  [[nodiscard]] uint8_t GetRowCount() const;

  /**
   * @brief Returns configured display column count.
   *
   * This value comes from the selected Geometry and is used for bounds checks
   * in cursor positioning.
   *
   * @retval uint8_t Column count for selected geometry.
   */
  [[nodiscard]] uint8_t GetColumnCount() const;

 private:
  hal_interface::ErrorCode DelayUs(uint32_t microseconds);
  hal_interface::ErrorCode DelayMs(uint32_t milliseconds);
  hal_interface::ErrorCode WriteCommand(uint8_t value);
  hal_interface::ErrorCode WriteData(uint8_t value);
  hal_interface::ErrorCode UpdateDisplayControl();
  hal_interface::ErrorCode InitializeDependencies();
  hal_interface::ErrorCode InitializeDefaultDisplayState();
  [[nodiscard]] uint8_t GetRowOffset(uint8_t row) const;
  static uint8_t ResolveRows(Geometry geometry);
  static uint8_t ResolveColumns(Geometry geometry);

  interface::Hd44780Bus &bus_;
  hal_interface::SoftwareTimer &timer_;
  Geometry geometry_;
  uint8_t rows_;
  uint8_t columns_;
  bool initialized_{false};
  bool display_enabled_{true};
  bool cursor_enabled_{false};
  bool blink_enabled_{false};
  bool entry_increment_{true};
  bool entry_shift_{false};
  uint8_t current_row_{0};
  uint8_t current_column_{0};

  static constexpr char kFirstPrintableChar{' '};
  static constexpr char kLastPrintableChar{0x7E};

  static constexpr uint8_t kCommandClear{0x01U};
  static constexpr uint8_t kCommandHome{0x02U};
  static constexpr uint8_t kCommandEntryModeSet{0x04U};
  static constexpr uint8_t kCommandDisplayControl{0x08U};
  static constexpr uint8_t kCommandCursorShift{0x10U};
  static constexpr uint8_t kCommandFunctionSet{0x20U};
  static constexpr uint8_t kCommandSetCgramAddress{0x40U};
  static constexpr uint8_t kCommandInit8Bits{0x30U};
  static constexpr uint8_t kCommandSetDdramAddress{0x80U};

  static constexpr uint8_t kFlagEntryIncrement{0x02U};
  static constexpr uint8_t kFlagEntryShift{0x01U};
  static constexpr uint8_t kFlagDisplayOn{0x04U};
  static constexpr uint8_t kFlagCursorOn{0x02U};
  static constexpr uint8_t kFlagBlinkOn{0x01U};
  static constexpr uint8_t kFlagShiftDisplay{0x08U};
  static constexpr uint8_t kFlagShiftRight{0x04U};
  static constexpr uint8_t kFlagFunction8Bits{0x10U};
  static constexpr uint8_t kFlagFunction2Lines{0x08U};

  static constexpr uint8_t kRowOffsetLine0{0x00U};
  static constexpr uint8_t kRowOffsetLine1{0x40U};
  static constexpr uint8_t kRowOffsetLine2{0x14U};
  static constexpr uint8_t kRowOffsetLine3{0x54U};

  static constexpr uint8_t kColumns8{8U};
  static constexpr uint8_t kColumns16{16U};
  static constexpr uint8_t kColumns20{20U};
  static constexpr uint8_t kColumns40{40U};

  static constexpr uint8_t kCgramSlotCount{8U};
  static constexpr uint8_t kCgramRowsPerSlot{kHd44780CgramRowsPerSlot};
  static constexpr uint8_t kCgramDataMask{0x1FU};
  static constexpr uint8_t kCgramSlotMask{0x07U};

  static constexpr uint32_t kDelayPowerOnMs{50};
  static constexpr uint32_t kDelayFunctionSetStepMs{5};
  static constexpr uint32_t kDelayFunctionSetRepeatUs{150};
  static constexpr uint32_t kDelayClearHomeUs{2000};
};

}  // namespace sfw::device::hd44780

#endif  // DEVICE_HD44780_HD44780_HPP