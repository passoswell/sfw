// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef DEVICE_SERIAL_EEPROM_INTERFACE_SERIAL_EEPROM_TRANSPORT_INTERFACE_HPP
#define DEVICE_SERIAL_EEPROM_INTERFACE_SERIAL_EEPROM_TRANSPORT_INTERFACE_HPP

#include <cstdint>
#include <span>

#include "hal_interface/error_code.hpp"

namespace sfw::device::serial_eeprom {

/**
 * @brief Abstract transport used by SerialEeprom.
 *
 * This interface hides the bus details used by serial EEPROM devices. The
 * command/address and data layout are handled by SerialEeprom and passed to
 * this layer as raw transfer buffers.
 *
 * Typical usage:
 * 1. Construct a concrete transport implementation.
 * 2. Call Initialize() before any transfer.
 * 3. Call WriteToAddress() or ReadFromAddress() for I/O.
 * 4. Call WaitUntilReady() after write cycles when required.
 * 5. Call Deinitialize() when transfers are no longer needed.
 */
class SerialEepromTransportInterface {
 public:
  SerialEepromTransportInterface() = default;
  SerialEepromTransportInterface(const SerialEepromTransportInterface&) =
      default;
  SerialEepromTransportInterface& operator=(
      const SerialEepromTransportInterface&) = default;
  SerialEepromTransportInterface(SerialEepromTransportInterface&&) = default;
  SerialEepromTransportInterface& operator=(SerialEepromTransportInterface&&) =
      default;

  /**
   * @brief Destructor
   */
  virtual ~SerialEepromTransportInterface() = default;

  /**
   * @brief Initializes the transport peripheral.
   *
   * Implementations should initialize their underlying bus dependencies.
   * This method must be called before WriteToAddress(), ReadFromAddress(), or
   * WaitUntilReady().
   *
   * @retval hal_interface::ErrorCode::kOk Initialization succeeded.
   * @retval hal_interface::ErrorCode::kError Initialization failed.
   */
  virtual hal_interface::ErrorCode Initialize() = 0;

  /**
   * @brief Deinitializes the transport peripheral.
   *
   * Implementations should release resources allocated by Initialize().
   * After this call, Initialize() must be called again before any transfer.
   *
   * @retval hal_interface::ErrorCode::kOk Deinitialization succeeded.
   * @retval hal_interface::ErrorCode::kError Deinitialization failed.
   */
  virtual hal_interface::ErrorCode Deinitialize() = 0;

  /**
   * @brief Returns whether the transport is initialized.
   *
   * @retval true Transport is initialized and ready.
   * @retval false Transport is not initialized.
   */
  virtual bool IsInitialized() = 0;

  /**
   * @brief Writes bytes to the EEPROM starting from @p address.
   *
   * Implementations must transmit the address preamble and then all bytes in
   * @p buffer according to the bus semantics.
   *
   * @param[in] address Start address inside the EEPROM memory map.
   * @param[in] address_size_bytes Number of bytes used to encode memory
   * addresses.
   * @param[in] buffer Data bytes to transmit.
   * @param[in] timeout_ms Transfer timeout in milliseconds.
   * @retval hal_interface::ErrorCode::kOk Write completed.
   * @retval hal_interface::ErrorCode::kInvalidArgument Invalid arguments.
   * @retval hal_interface::ErrorCode::kNotSupported Address width unsupported.
   * @retval hal_interface::ErrorCode::kTimeout Transfer timed out.
   * @retval hal_interface::ErrorCode::kError Transfer failed.
   */
  virtual hal_interface::ErrorCode WriteToAddress(
      uint32_t address, uint8_t address_size_bytes,
      std::span<const uint8_t> buffer, uint32_t timeout_ms) = 0;

  /**
   * @brief Reads bytes from the EEPROM starting from @p address.
   *
   * Implementations must transmit the address preamble and then read
   * @p buffer.size() bytes into @p buffer without breaking the protocol
   * sequence required by the bus.
   *
   * @param[in] address Start address inside the EEPROM memory map.
   * @param[in] address_size_bytes Number of bytes used to encode memory
   * addresses.
   * @param[out] buffer Destination bytes read from the target.
   * @param[out] buffer Destination bytes read from the target.
   * @param[in] timeout_ms Transfer timeout in milliseconds.
   * @retval hal_interface::ErrorCode::kOk Transfer completed.
   * @retval hal_interface::ErrorCode::kInvalidArgument Invalid arguments.
   * @retval hal_interface::ErrorCode::kNotSupported Address width unsupported.
   * @retval hal_interface::ErrorCode::kTimeout Transfer timed out.
   * @retval hal_interface::ErrorCode::kError Transfer failed.
   */
  virtual hal_interface::ErrorCode ReadFromAddress(uint32_t address,
                                                   uint8_t address_size_bytes,
                                                   std::span<uint8_t> buffer,
                                                   uint32_t timeout_ms) = 0;

  /**
   * @brief Waits until the target becomes ready for a new command.
   *
   * This method should be called after write transfers that trigger an
   * internal EEPROM programming cycle.
   *
   * @param[in] timeout_ms Maximum wait time in milliseconds.
   * @retval hal_interface::ErrorCode::kOk Target is ready.
   * @retval hal_interface::ErrorCode::kTimeout Ready wait timed out.
   * @retval hal_interface::ErrorCode::kError Transport or target failure.
   */
  virtual hal_interface::ErrorCode WaitUntilReady(uint32_t timeout_ms) = 0;
};

}  // namespace sfw::device::serial_eeprom

#endif  // DEVICE_SERIAL_EEPROM_INTERFACE_SERIAL_EEPROM_TRANSPORT_INTERFACE_HPP
