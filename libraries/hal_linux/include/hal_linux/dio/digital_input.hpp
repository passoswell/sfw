// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_LINUX_DIO_DIGITAL_INPUT_HPP
#define HAL_LINUX_DIO_DIGITAL_INPUT_HPP

#include <cstdint>
#include <string>
#include <string_view>

#include "hal_interface/digital_input.hpp"

extern "C" {
struct gpiod_chip;
struct gpiod_line;
}

namespace sfw::hal_linux {

/**
 * @brief Linux implementation of the hal_interface::DigitalInput interface.
 *
 * Uses libgpiod to request a DIO line as input and read logical levels. The
 * implementation opens a DIO chip by name and then requests the line offset
 * provided in the constructor.
 *
 * While libgpiod supports all line bias options, not all gpiochips may have
 * the necessary hardware to implement them. However, this implementation will
 * accept all possible configurations unconditionally. Since libgpiod does not
 * provide a way to query the hardware capabilities, nor will it necessarily
 * return an error if the requested bias is unsupported, please refer to your
 * platform or adapter's documentation prior to using this driver.
 *
 * Include path: `#include "hal_linux/dio/digital_input.hpp"`
 *
 * Typical usage:
 * 1. Construct DigitalInput with a line offset.
 * 2. Call Initialize() once before any Read() calls.
 * 3. Call Read() to sample the input level.
 * 4. Call Deinitialize() to release the line and close the chip.
 */
class DigitalInput final : public hal_interface::DigitalInput {
 public:
  /**
   * @brief Constructs a Linux digital input.
   *
   * Line bias options are platform dependent. Since there is no way to query
   * capabilities, nor will libgpiod return an error, this implementation will
   * accept all configuration combinations. Please refer to your platform or
   * adapter's documentation prior to using this driver.
   *
   * @param[in] device Name of the DIO chip (e.g. "/dev/gpiochip0").
   * @param[in] line_offset Line offset within the chip to sample.
   * @param[in] bias Internal pull resistor configuration for the line.
   */
  explicit DigitalInput(std::string device, uint32_t line_offset,
                        hal_interface::DigitalInput::LineBias bias =
                            hal_interface::DigitalInput::LineBias::kNone);

  /**
   * @brief Destructor
   */
  ~DigitalInput() override;

  DigitalInput(const DigitalInput&) = delete;
  DigitalInput& operator=(const DigitalInput&) = delete;
  DigitalInput(DigitalInput&&) = delete;
  DigitalInput& operator=(DigitalInput&&) = delete;

  /**
   * @brief Opens the DIO chip and requests the line as input.
   *
   * Opens the desired chip, acquires the line offset, and requests it as
   * input with the desired bias. Must be called before Read().
   *
   * Line bias options are platform dependent. Since there is no way to query
   * capabilities, nor will libgpiod return an error, this implementation will
   * accept all configuration combinations. Please refer to your platform or
   * adapter's documentation prior to using this driver.
   *
   * @retval ErrorCode::kOk    DIO is ready for input operations.
   * @retval ErrorCode::kError One or more libgpiod operations failed.
   */
  hal_interface::ErrorCode Initialize() override;

  /**
   * @brief Releases the line request and closes the chip.
   *
   * Releases libgpiod resources allocated during Initialize().
   *
   * @retval ErrorCode::kOk    DIO resources released successfully.
   * @retval ErrorCode::kError A libgpiod operation failed.
   */
  hal_interface::ErrorCode Deinitialize() override;

  /**
   * @brief Returns whether Initialize() completed successfully.
   *
   * @retval true  DIO is initialized and ready.
   * @retval false DIO is not initialized.
   */
  bool IsInitialized() override;

  /**
   * @brief Reads the current logical level from the input line.
   *
   * Samples the current pin value using libgpiod. The timeout parameter is
   * accepted to match the interface but is not used because this call is
   * immediate.
   *
   * @param[out] state Receives the current pin level: true for high, false
   * for low.
   * @param[in] timeout_ms Unused in this implementation.
   * @retval ErrorCode::kOk    Value read successfully.
   * @retval ErrorCode::kError DIO is not initialized or read failed.
   */
  hal_interface::ErrorCode Read(bool& state, uint32_t timeout_ms) override;

 private:
  hal_interface::ErrorCode CloseLineAndChip();
  [[nodiscard]] int GetBiasFlags() const;

  std::string device_;    ///< Name of the DIO chip (e.g. "/dev/gpiochip0").
  uint32_t line_offset_;  ///< Line offset within the chip to sample.
  hal_interface::DigitalInput::LineBias bias_;  ///< Internal pull resistor.
  gpiod_chip* chip_{nullptr};                   ///< DIO chip handle.
  gpiod_line* line_{nullptr};                   ///< DIO line handle.
  bool initialized_{false};  ///< True after successful Initialize().

  const char* kGpiodConsumerName_{"sfw_digital_input"};
  const std::string_view KDevPathPrefix{"/dev/"};
};

}  // namespace sfw::hal_linux

#endif  // HAL_LINUX_DIO_DIGITAL_INPUT_HPP