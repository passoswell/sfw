// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef DEVICE_PCF8574_PCF8574_HPP
#define DEVICE_PCF8574_PCF8574_HPP

#include <cstdint>

#include "hal_interface/error_code.hpp"
#include "hal_interface/i2c_controller.hpp"

namespace sfw::device::pcf8574 {

/**
 * @brief Possible I2C addresses for the PCF8574 and PCF8574A variants.
 */
enum class I2cAddress : uint8_t {
  k0x20h = 0x20,  ///< A2=0, A1=0, A0=0 on PCF8574
  k0x21h = 0x21,  ///< A2=0, A1=0, A0=1 on PCF8574
  k0x22h = 0x22,  ///< A2=0, A1=1, A0=0 on PCF8574
  k0x23h = 0x23,  ///< A2=0, A1=1, A0=1 on PCF8574
  k0x24h = 0x24,  ///< A2=1, A1=0, A0=0 on PCF8574
  k0x25h = 0x25,  ///< A2=1, A1=0, A0=1 on PCF8574
  k0x26h = 0x26,  ///< A2=1, A1=1, A0=0 on PCF8574
  k0x27h = 0x27,  ///< A2=1, A1=1, A0=1 on PCF8574
  k0x38h = 0x38,  ///< A2=0, A1=0, A0=0 on PCF8574A
  k0x39h = 0x39,  ///< A2=0, A1=0, A0=1 on PCF8574A
  k0x3Ah = 0x3A,  ///< A2=0, A1=1, A0=0 on PCF8574A
  k0x3Bh = 0x3B,  ///< A2=0, A1=1, A0=1 on PCF8574A
  k0x3Ch = 0x3C,  ///< A2=1, A1=0, A0=0 on PCF8574A
  k0x3Dh = 0x3D,  ///< A2=1, A1=0, A0=1 on PCF8574A
  k0x3Eh = 0x3E,  ///< A2=1, A1=1, A0=0 on PCF8574A
  k0x3Fh = 0x3F,  ///< A2=1, A1=1, A0=1 on PCF8574A
};

/**
 * @brief Provides access to the PCF8574 I/O expander.
 *
 * This class wraps one-byte PCF8574 port transactions over an I2C controller.
 * Note that the DIOs on this device are quasi-bidirectional, meaning that
 * writing a high level to a pin does not drive it high, but rather releases it
 * to be pulled high by an external resistor.
 * The interruption pin, responsible for signaling input changes, is not
 * supported by this class.
 * This class is compatible with both the PCF8574 and PCF8574A variants, which
 * differ only in their I2C base addresses.
 * The possible I2C addresses for both variants are defined in the I2cAddress
 * enum.
 * No mutex is used to protect concurrent access to the shared I2C controller,
 * so concurrent access must be managed externally. This might change in newer
 * versions.
 *
 * Include path: `#include "device/pcf8574/pcf8574.hpp"`
 *
 * Typical usage:
 * 1. Construct the class with an initialized I2C controller and 7-bit address.
 * 2. Call Initialize() once to verify that the target acknowledges on the bus.
 * 3. Use ReadPin()/WritePin() for single-bit access.
 * 4. Use ReadPort()/WritePort() for full 8-bit port transfers.
 */
class Pcf8574 {
 public:
  /**
   * @brief Constructs a PCF8574 access object.
   *
   * @param[in] i2c I2C controller used for all transfers.
   * @param[in] address 7-bit PCF8574 target address.
   */
  Pcf8574(hal_interface::I2cController &i2c, uint8_t address,
          uint8_t initial_port_state = Pcf8574::kInitialValues);

  Pcf8574(const Pcf8574 &) = delete;
  Pcf8574 &operator=(const Pcf8574 &) = delete;
  Pcf8574(Pcf8574 &&) = delete;
  Pcf8574 &operator=(Pcf8574 &&) = delete;

  /**
   * @brief Destructor
   */
  ~Pcf8574() = default;

  /**
   * @brief Initializes the IO expander.
   *
   * Checks that the PCF8574 target is reachable on the I2C bus.
   * Then, configure all DIOs as inputs (high state) by writing 0xFF to the
   * port since this is the default state of the pins on power-up. Keep in mind
   * that having unused floating pins configured as inputs may trigger the
   * interrupt pin due to noise.
   *
   * @retval hal_interface::ErrorCode::kOk Target detected.
   * @retval hal_interface::ErrorCode::kTimeout Detection timed out.
   * @retval hal_interface::ErrorCode::kError Target not reachable.
   */
  hal_interface::ErrorCode Initialize();

  /**
   * @brief Reads one GPIO level from the expander.
   *
   * @param[in] pin Pin index in range [0, 7].
   * @param[out] level Read logic level.
   * @param[in] timeout_ms Maximum time to wait for the transfer, in
   * milliseconds.
   * @retval hal_interface::ErrorCode::kOk Pin level was read.
   * @retval hal_interface::ErrorCode::kOutOfRange Pin index is invalid.
   * @retval hal_interface::ErrorCode::kTimeout I2C transfer timed out.
   * @retval hal_interface::ErrorCode::kError I2C transfer failed.
   */
  hal_interface::ErrorCode ReadPin(uint8_t pin, bool &level,
                                   uint32_t timeout_ms);

  /**
   * @brief Writes one GPIO level to the expander.
   *
   * The implementation performs a read-modify-write on the 8-bit port.
   *
   * @param[in] pin Pin index in range [0, 7].
   * @param[in] level Logic level to write.
   * @param[in] timeout_ms Maximum time to wait for each transfer, in
   * milliseconds.
   * @retval hal_interface::ErrorCode::kOk Pin level was written.
   * @retval hal_interface::ErrorCode::kOutOfRange Pin index is invalid.
   * @retval hal_interface::ErrorCode::kTimeout I2C transfer timed out.
   * @retval hal_interface::ErrorCode::kError I2C transfer failed.
   */
  hal_interface::ErrorCode WritePin(uint8_t pin, bool level,
                                    uint32_t timeout_ms);

  /**
   * @brief Reads the full 8-bit port state.
   *
   * @param[out] value Byte read from the expander port.
   * @param[in] timeout_ms Maximum time to wait for the transfer, in
   * milliseconds.
   * @retval hal_interface::ErrorCode::kOk Port was read.
   * @retval hal_interface::ErrorCode::kInvalidArgument Address is not 7-bit.
   * @retval hal_interface::ErrorCode::kTimeout I2C transfer timed out.
   * @retval hal_interface::ErrorCode::kError I2C transfer failed.
   */
  hal_interface::ErrorCode ReadPort(uint8_t &value, uint32_t timeout_ms);

  /**
   * @brief Writes the full 8-bit port state.
   *
   * @param[in] value Byte to write to the expander port.
   * @param[in] timeout_ms Maximum time to wait for the transfer, in
   * milliseconds.
   * @retval hal_interface::ErrorCode::kOk Port was written.
   * @retval hal_interface::ErrorCode::kTimeout I2C transfer timed out.
   * @retval hal_interface::ErrorCode::kError I2C transfer failed.
   */
  hal_interface::ErrorCode WritePort(uint8_t value, uint32_t timeout_ms);

 private:
  hal_interface::I2cController &i2c_;  ///< I2C controller used in all transfers
  uint8_t address_;             ///< 7-bit I2C address of the PCF8574 target
  uint8_t initial_port_state_;  ///< Default state with all pins as inputs
  bool initialized_{false};
  uint8_t port_state_{0};  ///< Port state for read-modify-write operations

  static const uint8_t kPinCount{8};         ///< Number of DIOs on the expander
  static const uint32_t kI2cTimeoutMs{100};  ///< 100 milliseconds
  static const uint8_t kInitialValues{0xFF};  ///< Default, all pins as inputs
};

}  // namespace sfw::device::pcf8574

#endif  // DEVICE_PCF8574_PCF8574_HPP