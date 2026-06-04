// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef DEVICE_SERIAL_EEPROM_SERIAL_EEPROM_I2C_TRANSPORT_HPP
#define DEVICE_SERIAL_EEPROM_SERIAL_EEPROM_I2C_TRANSPORT_HPP

#include <cstdint>
#include <span>

#include "device/serial_eeprom/interface/serial_eeprom_transport_interface.hpp"
#include "hal_interface/error_code.hpp"
#include "hal_interface/i2c_controller.hpp"
#include "hal_interface/memory.hpp"
#include "hal_interface/software_timer.hpp"

namespace sfw::device::serial_eeprom {

/**
 * @brief I2C transport implementation for SerialEeprom.
 *
 * This class adapts hal_interface::I2cController to
 * SerialEepromTransportInterface.
 *
 * Include path:
 * #include "device/serial_eeprom/serial_eeprom_i2c_transport.hpp"
 *
 * Typical usage:
 * 1. Construct with a hal_interface::I2cController, SoftwareTimer and target
 * address.
 * 2. Call Initialize().
 * 3. Inject into SerialEeprom.
 * 4. Use SerialEeprom for Read()/Write().
 * 5. Call Deinitialize() when no longer required.
 */
class SerialEepromI2cTransport final : public SerialEepromTransportInterface {
 public:
  /**
   * @brief Constructs an I2C serial EEPROM transport.
   *
   * @param[in] i2c I2C controller used for all transfers.
   * @param[in] target_address Target I2C address.
   * @param[in] is_10bit_address True for 10-bit I2C addressing mode.
   * @param[in] timer Software timer used for poll timing.
   * @param[in] ready_poll_interval_ms Delay between ready polls.
   * @param[in] ready_poll_attempt_timeout_ms Timeout for each Detect attempt.
   */
  SerialEepromI2cTransport(hal_interface::I2cController& i2c,
                           uint16_t target_address, bool is_10bit_address,
                           hal_interface::SoftwareTimer& timer,
                           uint32_t ready_poll_interval_ms = 1U,
                           uint32_t ready_poll_attempt_timeout_ms = 1U);

  SerialEepromI2cTransport(const SerialEepromI2cTransport&) = delete;
  SerialEepromI2cTransport& operator=(const SerialEepromI2cTransport&) = delete;
  SerialEepromI2cTransport(SerialEepromI2cTransport&&) = delete;
  SerialEepromI2cTransport& operator=(SerialEepromI2cTransport&&) = delete;

  /**
   * @brief Destructor
   */
  ~SerialEepromI2cTransport() override = default;

  /**
   * @brief Initializes the transport and injected dependencies.
   *
   * Initializes the injected I2C controller and software timer when they are
   * not already initialized.
   *
   * @retval hal_interface::ErrorCode::kOk Initialization succeeded.
   * @retval hal_interface::ErrorCode::kInvalidArgument Configuration is
   * invalid.
   * @retval hal_interface::ErrorCode::kError Dependency initialization failed.
   */
  hal_interface::ErrorCode Initialize() override;

  /**
   * @brief Deinitializes this transport instance.
   *
   * This call only updates this class state and does not deinitialize
   * injected dependencies.
   *
   * @retval hal_interface::ErrorCode::kOk Deinitialization succeeded.
   */
  hal_interface::ErrorCode Deinitialize() override;

  /**
   * @brief Returns whether this transport is initialized.
   *
   * @retval true Transport is initialized.
   * @retval false Transport is not initialized.
   */
  bool IsInitialized() override;

  /**
   * @brief Writes bytes to EEPROM memory over I2C.
   *
   * @param[in] address Address to send before data.
   * @param[in] address_size_bytes Number of bytes used to encode memory
   * addresses.
   * @param[in] buffer Data bytes to write.
   * @param[in] timeout_ms Maximum time for the transfer, in milliseconds.
   * @retval hal_interface::ErrorCode::kOk Transfer completed.
   * @retval hal_interface::ErrorCode::kInvalidArgument Invalid arguments.
   * @retval hal_interface::ErrorCode::kNotSupported Address width unsupported.
   * @retval hal_interface::ErrorCode::kTimeout Transfer timed out.
   * @retval hal_interface::ErrorCode::kError Transfer failed.
   */
  hal_interface::ErrorCode WriteToAddress(uint32_t address,
                                          uint8_t address_size_bytes,
                                          std::span<const uint8_t> buffer,
                                          uint32_t timeout_ms) override;

  /**
   * @brief Reads bytes from EEPROM memory over I2C.
   *
   * @param[in] address Address to send before reading.
   * @param[in] address_size_bytes Number of bytes used to encode memory
   * addresses.
   * @param[out] buffer Buffer that receives read bytes.
   * @param[in] timeout_ms Maximum time for the transaction, in milliseconds.
   * @retval hal_interface::ErrorCode::kOk Transaction completed.
   * @retval hal_interface::ErrorCode::kInvalidArgument Invalid arguments.
   * @retval hal_interface::ErrorCode::kNotSupported Address width unsupported.
   * @retval hal_interface::ErrorCode::kTimeout Transaction timed out.
   * @retval hal_interface::ErrorCode::kError Transaction failed.
   */
  hal_interface::ErrorCode ReadFromAddress(uint32_t address,
                                           uint8_t address_size_bytes,
                                           std::span<uint8_t> buffer,
                                           uint32_t timeout_ms) override;

  /**
   * @brief Polls the target until it acknowledges or timeout expires.
   *
   * Uses the injected software timer to enforce an overall timeout and poll
   * interval.
   *
   * @param[in] timeout_ms Maximum wait time, in milliseconds.
   * @retval hal_interface::ErrorCode::kOk Target is ready.
   * @retval hal_interface::ErrorCode::kInvalidArgument Invalid timeout.
   * @retval hal_interface::ErrorCode::kTimeout Ready wait timed out.
   * @retval hal_interface::ErrorCode::kError Polling failed.
   */
  hal_interface::ErrorCode WaitUntilReady(uint32_t timeout_ms) override;

 private:
  hal_interface::I2cController& i2c_;
  uint16_t target_address_{0U};
  bool is_10bit_address_{false};
  hal_interface::SoftwareTimer& timer_;
  uint32_t ready_poll_interval_ms_{1U};
  uint32_t ready_poll_attempt_timeout_ms_{1U};
  bool initialized_{false};
};

}  // namespace sfw::device::serial_eeprom

#endif  // DEVICE_SERIAL_EEPROM_SERIAL_EEPROM_I2C_TRANSPORT_HPP
