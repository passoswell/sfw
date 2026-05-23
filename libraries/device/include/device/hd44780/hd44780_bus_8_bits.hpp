// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef DEVICE_HD44780_HD44780_BUS_8_BITS_HPP
#define DEVICE_HD44780_HD44780_BUS_8_BITS_HPP

#include <cstdint>

#include "device/hd44780/interface/hd44780_bus_interface.hpp"
#include "hal_interface/digital_output.hpp"
#include "hal_interface/software_timer.hpp"

namespace sfw::device::hd44780 {

/**
 * @brief HD44780 write-only 8-bit parallel bus implementation.
 *
 * This class implements Hd44780Bus for the full 8-data-line bus.
 * It uses a software timer to enforce setup/hold and enable pulse widths.
 * This class considers the write/read pin as optional. If it is provided,
 * this class will set it to low to enable writes only. If it is not provided,
 * it is considered as tied to ground in hardware or managed externally in
 * software to configure the display in write-only mode.
 * All bus operations are blocking and return only after the operation is
 * complete.
 * No timeouts are configurable for the overall methods operation. However, a
 * reasonable default timeout value is used with internal call to functions of
 * the injected bus and timer backends, which can be adjusted by modifying the
 * kPinTimeoutMs constant in the bus implementations.
 *
 * Include path: `#include "device/hd44780/hd44780_bus_8_bits.hpp"`
 *
 * Typical usage:
 * 1. Construct with RS, EN, D0..D7 digital outputs and a software timer. WR pin
 * is considered as connected to ground in hardware, which configures the
 * display in write mode only.
 * 2. Call Initialize() once to prepare GPIO lines and timer backend.
 * 3. Use WriteCommand() and WriteData() to transmit bytes.
 * 4. Call Deinitialize() when the object is no longer needed.
 */
class Hd44780Bus8Bits final : public interface::Hd44780Bus {
 public:
  /**
   * @brief Constructs an 8-bit HD44780 bus object.
   *
   * @param[in] rs_pin Register-select output pin.
   * @param[in] en_pin Enable output pin.
   * @param[in] data0_pin Data bit 0 output pin.
   * @param[in] data1_pin Data bit 1 output pin.
   * @param[in] data2_pin Data bit 2 output pin.
   * @param[in] data3_pin Data bit 3 output pin.
   * @param[in] data4_pin Data bit 4 output pin.
   * @param[in] data5_pin Data bit 5 output pin.
   * @param[in] data6_pin Data bit 6 output pin.
   * @param[in] data7_pin Data bit 7 output pin.
   * @param[in] timer Software timer used to satisfy bus timing.
   * @param[in] wr_pin Optional parameter for the write/read pin.
   */
  Hd44780Bus8Bits(hal_interface::DigitalOutput &rs_pin,
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
                  hal_interface::DigitalOutput *wr_pin = nullptr);

  Hd44780Bus8Bits(const Hd44780Bus8Bits &) = delete;
  Hd44780Bus8Bits &operator=(const Hd44780Bus8Bits &) = delete;
  Hd44780Bus8Bits(Hd44780Bus8Bits &&) = delete;
  Hd44780Bus8Bits &operator=(Hd44780Bus8Bits &&) = delete;

  /**
   * @brief Destructor
   */
  ~Hd44780Bus8Bits() override = default;

  /**
   * @brief Initializes all bus dependencies and drives idle levels.
   *
   * Implementation details:
   * - If WR pin is provided, drives it low to enable write mode.
   * - Initializes timer and all output pins if they are not initialized yet.
   * - Drives EN and RS low and clears data pins.
   *
   * @retval hal_interface::ErrorCode::kOk Bus initialized.
   * @retval hal_interface::ErrorCode::kError Initialization failed.
   */
  hal_interface::ErrorCode Initialize() override;

  /**
   * @brief Marks the bus as deinitialized.
   *
   * Implementation details:
   * - Drives EN and RS low and clears data pins.
   * - Deinitializes injected output pins and software timer dependencies.
   *
   * @retval hal_interface::ErrorCode::kOk Bus deinitialized.
   * @retval hal_interface::ErrorCode::kError Pin write failed.
   */
  hal_interface::ErrorCode Deinitialize() override;

  /**
   * @brief Returns whether this bus instance is initialized.
   *
   * Implementation details:
   * - Returns the internal state updated by Initialize()/Deinitialize().
   *
   * @retval true Bus is initialized.
   * @retval false Bus is not initialized.
   */
  bool IsInitialized() override;

  /**
   * @brief Writes one instruction byte with RS low.
   *
   * Implementation details:
   * - Places all 8 bits on D0..D7 and pulses EN using the timer.
   *
   * @param[in] value Instruction value.
   * @retval hal_interface::ErrorCode::kOk Command written.
   * @retval hal_interface::ErrorCode::kError Bus unavailable or write failed.
   */
  hal_interface::ErrorCode WriteCommand(uint8_t value) override;

  /**
   * @brief Writes one data byte with RS high.
   *
   * Implementation details:
   * - Places all 8 bits on D0..D7 and pulses EN using the timer.
   *
   * @param[in] value Data value.
   * @retval hal_interface::ErrorCode::kOk Data written.
   * @retval hal_interface::ErrorCode::kError Bus unavailable or write failed.
   */
  hal_interface::ErrorCode WriteData(uint8_t value) override;

  /**
   * @brief Returns the bus width for this implementation.
   *
   * @retval interface::Hd44780Bus::Width::k8Bits Always for this class.
   */
  [[nodiscard]] interface::Hd44780Bus::Width GetWidth() const override;

 private:
  hal_interface::ErrorCode DelayUs(uint32_t microseconds);
  static hal_interface::ErrorCode InitializePin(
      hal_interface::DigitalOutput &pin, bool state);
  hal_interface::ErrorCode PulseEnable();
  hal_interface::ErrorCode WriteByte(bool rs_state, uint8_t value);
  hal_interface::ErrorCode SetDataPins(uint8_t value);

  hal_interface::DigitalOutput &rs_;
  hal_interface::DigitalOutput &en_;
  hal_interface::DigitalOutput &d0_;
  hal_interface::DigitalOutput &d1_;
  hal_interface::DigitalOutput &d2_;
  hal_interface::DigitalOutput &d3_;
  hal_interface::DigitalOutput &d4_;
  hal_interface::DigitalOutput &d5_;
  hal_interface::DigitalOutput &d6_;
  hal_interface::DigitalOutput &d7_;
  hal_interface::SoftwareTimer &timer_;
  hal_interface::DigitalOutput *wr_;
  bool initialized_{false};

  static constexpr uint32_t kPinTimeoutMs{100};
  static constexpr uint32_t kDelayEnablePulseUs{5};
  static constexpr uint32_t kDelayWriteCycleUs{50};
  static constexpr uint32_t kDelayPowerOnUs{50000};
  static constexpr uint32_t kDelayFunctionSetStepUs{5000};
  static constexpr uint32_t kDelayFunctionSetRepeatUs{150};
  static constexpr uint8_t kCommandInit8Bits{0x30U};

  static constexpr uint8_t kBit0Mask{0x01U};
  static constexpr uint8_t kBit1Mask{0x02U};
  static constexpr uint8_t kBit2Mask{0x04U};
  static constexpr uint8_t kBit3Mask{0x08U};
  static constexpr uint8_t kBit4Mask{0x10U};
  static constexpr uint8_t kBit5Mask{0x20U};
  static constexpr uint8_t kBit6Mask{0x40U};
  static constexpr uint8_t kBit7Mask{0x80U};
};

}  // namespace sfw::device::hd44780

#endif  // DEVICE_HD44780_HD44780_BUS_8_BITS_HPP