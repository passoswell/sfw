// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_LINUX_DIO_DIGITAL_OUTPUT_HPP
#define HAL_LINUX_DIO_DIGITAL_OUTPUT_HPP

#include <cstdint>
#include <string>
#include <string_view>

#include "hal_interface/digital_output.hpp"

extern "C" {
struct gpiod_chip;
struct gpiod_line;
}

namespace sfw::hal_linux {

/**
 * @brief Linux implementation of the hal_interface::DigitalOutput
 * interface.
 *
 * Uses libgpiod to request a DIO line as output and drive logical levels.
 * The implementation opens a DIO chip by name and then requests the line
 * offset provided in the constructor.
 *
 * Include path: `#include <hal_linux/dio/digital_output.hpp>`
 *
 * Typical usage:
 * 1. Construct DigitalOutput with a line offset and optional initial state.
 * 2. Call Initialize() once before any Write() or Toggle() calls.
 * 3. Call Write() to drive a specific logical level on the DIO.
 * 4. Call Toggle() to invert the current logical level.
 * 5. Call Deinitialize() to release the line and close the chip.
 */
class DigitalOutput final : public hal_interface::DigitalOutput {
 public:
  /**
   * @brief Constructs a Linux digital output.
   *
   * @param[in] device Name of the DIO chip (e.g. "/dev/gpiochip0").
   * @param[in] line_offset Line offset within the chip to control.
   * @param[in] initial_state Initial logical level to drive after
   * initialization.
   * @param[in] line_drive Requested output drive mode for the line.
   * @param[in] line_bias Requested internal bias for the line.
   */
  explicit DigitalOutput(std::string device, uint32_t line_offset,
                         bool initial_state,
                         hal_interface::DigitalOutput::LineDrive line_drive =
                             hal_interface::DigitalOutput::LineDrive::kPushPull,
                         hal_interface::DigitalOutput::LineBias line_bias =
                             hal_interface::DigitalOutput::LineBias::kDisable);

  /**
   * @brief Destructor
   */
  ~DigitalOutput() override;

  DigitalOutput(const DigitalOutput&) = delete;
  DigitalOutput& operator=(const DigitalOutput&) = delete;
  DigitalOutput(DigitalOutput&&) = delete;
  DigitalOutput& operator=(DigitalOutput&&) = delete;

  /**
   * @brief Opens the DIO chip and requests the line as output.
   *
   * Opens the desired chip, acquire the line offset, and request it as output
   * with the desired initial state. Must be called before Write() and Toggle().
   *
   * @retval ErrorCode::kOk    DIO is ready for output operations.
   * @retval ErrorCode::kError One or more libgpiod operations failed.
   */
  hal_interface::ErrorCode Initialize() override;

  /**
   * @brief Releases the line request and closes the chip.
   *
   * Releases libgpiod resources allocated during Initialize().
   * The DIO is put in its initial state before releasing resources, but there
   * is no guarantee the kernel will keep it in that state after release.
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
   * @brief Writes a logical level to the digital output.
   *
   * Sets the requested output line value. The timeout parameter is accepted to
   * match the interface but is not used because this call is immediate.
   * Since this is running on linux, real time behavior is not to be expected.
   *
   * @param[in] state Desired logical level: true for high, false for low.
   * @param[in] timeout_ms Unused in this implementation.
   * @retval ErrorCode::kOk    Value written successfully.
   * @retval ErrorCode::kError DIO is not initialized or write failed.
   */
  hal_interface::ErrorCode Write(bool state, uint32_t timeout_ms) override;

  /**
   * @brief Inverts the current DIO logical level.
   *
   * Reads the current value and writes the opposite level via Write(). Requires
   * Initialize() to have completed first.
   *
   * @param[in] timeout_ms Forwarded to Write() for API compatibility.
   * @retval ErrorCode::kOk    DIO value toggled successfully.
   * @retval ErrorCode::kError DIO is not initialized or I/O failed.
   */
  hal_interface::ErrorCode Toggle(uint32_t timeout_ms) override;

 private:
  hal_interface::ErrorCode CloseLineAndChip();

  std::string device_;    ///< Name of the DIO chip (e.g. "/dev/gpiochip0").
  uint32_t line_offset_;  ///< Line offset within the chip to control.
  bool initial_state_;    ///< Initial output level after initialization.
  LineDrive line_drive_;  ///< Requested output drive mode.
  LineBias line_bias_;    ///< Requested line bias mode.
  gpiod_chip* chip_{nullptr};  ///< DIO chip handle
  gpiod_line* line_{nullptr};  ///< DIO line handle
  bool initialized_{false};    ///< True after successful Initialize().
  bool state_{false};          ///< Current output level

  const char* kGpiodConsumerName_{"sfw_digital_output"};
  const std::string_view KDevPathPrefix{"/dev/"};
};

}  // namespace sfw::hal_linux

#endif  // HAL_LINUX_DIO_DIGITAL_OUTPUT_HPP
