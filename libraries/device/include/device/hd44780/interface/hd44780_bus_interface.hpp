// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef DEVICE_HD44780_INTERFACE_HD44780_BUS_INTERFACE_HPP
#define DEVICE_HD44780_INTERFACE_HD44780_BUS_INTERFACE_HPP

#include <cstdint>

#include "hal_interface/error_code.hpp"

namespace sfw::device::hd44780::interface {

/**
 * @brief Abstract write-only HD44780 parallel bus interface.
 *
 * This interface encapsulates the physical electrical link between the
 * Hd44780 class and a display controller. Implementations are expected to
 * control RW, RS, EN and data pins and to satisfy HD44780 timing. Read
 * operations are intentionally not provided.
 * All bus operations are expected to be blocking and to return only after the
 * operation is complete.
 *
 * Include path:
 * `#include "device/hd44780/interface/ihd44780_bus_interface.hpp"`
 *
 * Typical usage:
 * 1. Construct a concrete bus implementation with required DigitalOutput and
 * timer dependencies.
 * 2. Call Initialize() once before any command or data write.
 * 3. Use WriteCommand() and WriteData() from the higher-level Hd44780 driver.
 * 4. Call Deinitialize() when the bus instance is no longer needed.
 */
class Hd44780Bus {
 public:
  /**
   * @brief Bus width options supported by HD44780-compatible displays.
   */
  enum class Width : uint8_t {
    k4Bits = 4,
    k8Bits = 8,
  };

  Hd44780Bus() = default;
  Hd44780Bus(const Hd44780Bus &) = default;
  Hd44780Bus &operator=(const Hd44780Bus &) = default;
  Hd44780Bus(Hd44780Bus &&) = default;
  Hd44780Bus &operator=(Hd44780Bus &&) = default;

  /**
   * @brief Destructor
   */
  virtual ~Hd44780Bus() = default;

  /**
   * @brief Initializes the electrical bus backend.
   *
   * Expected behavior for implementations:
   * - Configure all required output lines and any timing backend.
   * - Drive the bus to an idle write-safe state.
   *
   * Call dependency:
   * - Must be called and succeed before WriteCommand() or WriteData().
   *
   * @retval hal_interface::ErrorCode::kOk Bus initialized.
   * @retval hal_interface::ErrorCode::kError Initialization failed.
   */
  virtual hal_interface::ErrorCode Initialize() = 0;

  /**
   * @brief Deinitializes the electrical bus backend.
   *
   * Expected behavior for implementations:
   * - Leave the bus in an inactive state and mark it unavailable.
   *
   * Call dependency:
   * - After this call, WriteCommand() and WriteData() must fail until
   * Initialize() is called again.
   *
   * @retval hal_interface::ErrorCode::kOk Bus deinitialized.
   * @retval hal_interface::ErrorCode::kError Deinitialization failed.
   */
  virtual hal_interface::ErrorCode Deinitialize() = 0;

  /**
   * @brief Returns whether the bus backend is initialized.
   *
   * Expected behavior for implementations:
   * - Report true only after a successful Initialize().
   *
   * Call dependency:
   * - Can be called at any time.
   *
   * @retval true Bus is initialized.
   * @retval false Bus is not initialized.
   */
  virtual bool IsInitialized() = 0;

  /**
   * @brief Writes one HD44780 instruction byte.
   *
   * Expected behavior for implementations:
   * - Assert command mode (RS low), transmit one byte and pulse EN with
   * compliant timing.
   *
   * Call dependency:
   * - Requires a successful Initialize().
   *
   * @param[in] value Instruction value to transfer.
   * @retval hal_interface::ErrorCode::kOk Command written.
   * @retval hal_interface::ErrorCode::kError Bus unavailable or write failed.
   */
  virtual hal_interface::ErrorCode WriteCommand(uint8_t value) = 0;

  /**
   * @brief Writes one HD44780 data byte.
   *
   * Expected behavior for implementations:
   * - Assert data mode (RS high), transmit one byte and pulse EN with
   * compliant timing.
   *
   * Call dependency:
   * - Requires a successful Initialize().
   *
   * @param[in] value Data value to transfer.
   * @retval hal_interface::ErrorCode::kOk Data written.
   * @retval hal_interface::ErrorCode::kError Bus unavailable or write failed.
   */
  virtual hal_interface::ErrorCode WriteData(uint8_t value) = 0;

  /**
   * @brief Returns the hardware bus width.
   *
   * Expected behavior for implementations:
   * - Return k4Bits for 4-bit buses and k8Bits for 8-bit buses.
   *
   * Call dependency:
   * - Can be called before or after Initialize().
   *
   * @retval Width::k4Bits 4-bit parallel bus.
   * @retval Width::k8Bits 8-bit parallel bus.
   */
  [[nodiscard]] virtual Width GetWidth() const = 0;
};

}  // namespace sfw::device::hd44780::interface

#endif  // DEVICE_HD44780_INTERFACE_HD44780_BUS_INTERFACE_HPP