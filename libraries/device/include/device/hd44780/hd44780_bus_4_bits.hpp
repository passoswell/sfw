// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef DEVICE_HD44780_HD44780_BUS_4_BITS_HPP
#define DEVICE_HD44780_HD44780_BUS_4_BITS_HPP

#include <cstdint>

#include "device/hd44780/interface/hd44780_bus_interface.hpp"
#include "hal_interface/digital_output.hpp"
#include "hal_interface/software_timer.hpp"

namespace sfw::device::hd44780 {

/**
 * @brief HD44780 write-only 4-bit parallel bus implementation.
 *
 * This class implements Hd44780Bus for D4..D7-only transfers.
 * It performs the HD44780 4-bit wake-up sequence and uses a software timer
 * to enforce setup/hold and enable pulse widths.
 * This class considers the write/read pin as optional. If it is provided,
 * this class will set it to low to enable writes only. If it is not provided,
 * it is considered as tied to ground in hardware or managed externally in
 * software to configure the display in write-only mode.
 * All bus operations areblocking and return only after the operation is
 * complete.
 * No timeouts are configurable for the overall methods operation. However, a
 * reasonable default timeout value is used with internal call to functions of
 * the injected bus and timer backends, which can be adjusted by modifying the
 * kPinTimeoutMs constant in the bus implementations.
 *
 * Include path: `#include "device/hd44780/hd44780_bus_4_bits.hpp"`
 *
 * Typical usage:
 * 1. Construct with RS, EN, D4..D7 digital outputs and a software timer.
 * 2. Call Initialize() once to run the 4-bit startup handshake.
 * 3. Use WriteCommand() and WriteData() to transmit bytes.
 * 4. Call Deinitialize() when the object is no longer needed.
 */
class Hd44780Bus4Bits final : public interface::Hd44780Bus {
 public:
  /**
   * @brief Constructs a 4-bit HD44780 bus object.
   *
   * @param[in] rs_pin Register-select output pin.
   * @param[in] en_pin Enable output pin.
   * @param[in] data4_pin Data bit 4 output pin.
   * @param[in] data5_pin Data bit 5 output pin.
   * @param[in] data6_pin Data bit 6 output pin.
   * @param[in] data7_pin Data bit 7 output pin.
   * @param[in] timer Software timer used to satisfy bus timing.
   * @param[in] wr_pin Optional parameter for the write/read pin.
   */
  Hd44780Bus4Bits(hal_interface::DigitalOutput &rs_pin,
                  hal_interface::DigitalOutput &en_pin,
                  hal_interface::DigitalOutput &data4_pin,
                  hal_interface::DigitalOutput &data5_pin,
                  hal_interface::DigitalOutput &data6_pin,
                  hal_interface::DigitalOutput &data7_pin,
                  hal_interface::SoftwareTimer &timer,
                  hal_interface::DigitalOutput *wr_pin = nullptr);

  Hd44780Bus4Bits(const Hd44780Bus4Bits &) = delete;
  Hd44780Bus4Bits &operator=(const Hd44780Bus4Bits &) = delete;
  Hd44780Bus4Bits(Hd44780Bus4Bits &&) = delete;
  Hd44780Bus4Bits &operator=(Hd44780Bus4Bits &&) = delete;

  /**
   * @brief Destructor
   */
  ~Hd44780Bus4Bits() override = default;

  /**
   * @brief Initializes all bus dependencies and performs 4-bit wake-up.
   *
   * Implementation details:
   * - If WR pin is provided, drives it low to enable write mode.
   * - Initializes timer and required output pins.
   * - Drives EN and RS low and clears D4..D7.
   * - Executes the HD44780 reset-to-4-bit startup sequence.
   *
   * @retval hal_interface::ErrorCode::kOk Bus initialized.
   * @retval hal_interface::ErrorCode::kError Initialization failed.
   */
  hal_interface::ErrorCode Initialize() override;

  /**
   * @brief Marks the bus as deinitialized.
   *
   * Implementation details:
   * - Drives EN and RS low and clears D4..D7.
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
   * - Splits byte into high then low nibble over D4..D7.
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
   * - Splits byte into high then low nibble over D4..D7.
   *
   * @param[in] value Data value.
   * @retval hal_interface::ErrorCode::kOk Data written.
   * @retval hal_interface::ErrorCode::kError Bus unavailable or write failed.
   */
  hal_interface::ErrorCode WriteData(uint8_t value) override;

  /**
   * @brief Returns the bus width for this implementation.
   *
   * @retval Width::k4Bits Always for this class.
   */
  [[nodiscard]] Width GetWidth() const override;

 private:
  hal_interface::ErrorCode DelayUs(uint32_t microseconds);
  static hal_interface::ErrorCode InitializePin(
      hal_interface::DigitalOutput &pin, bool state);
  hal_interface::ErrorCode PulseEnable();
  hal_interface::ErrorCode WriteByte(bool rs_state, uint8_t value);
  hal_interface::ErrorCode WriteNibble(uint8_t nibble);
  hal_interface::ErrorCode WriteInitializationNibble(uint8_t nibble);

  hal_interface::DigitalOutput &rs_;
  hal_interface::DigitalOutput &en_;
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
  static constexpr uint32_t kDelayInitStep1Us{4500};
  static constexpr uint32_t kDelayInitStep2Us{4500};
  static constexpr uint32_t kDelayInitStep3Us{150};

  static constexpr uint8_t kNibbleMask{0x0FU};
  static constexpr uint8_t kBit0Mask{0x01U};
  static constexpr uint8_t kBit1Mask{0x02U};
  static constexpr uint8_t kBit2Mask{0x04U};
  static constexpr uint8_t kBit3Mask{0x08U};
};

}  // namespace sfw::device::hd44780

#endif  // DEVICE_HD44780_HD44780_BUS_4_BITS_HPP