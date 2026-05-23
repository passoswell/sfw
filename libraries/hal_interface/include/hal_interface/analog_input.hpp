// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_INTERFACE_ANALOG_INPUT_HPP
#define HAL_INTERFACE_ANALOG_INPUT_HPP

#include <cstdint>
#include <span>

#include "hal_interface/error_code.hpp"

namespace sfw::hal_interface {

/**
 * @brief Abstract interface for analog input (ADC) peripherals.
 *
 * Provides methods to start and stop conversions and to retrieve results in
 * either normalized floating-point form or raw ADC counts.
 * All the methods on this interface are synchronous / blocking. This means
 * they only return if the operation is finished or if an error occurred.
 *
 * Typical usage:
 * 1. Call Initialize() once to configure the peripheral.
 * 2. Call Start() to begin conversions.
 * 3. Call Read() for normalized values or ReadRaw() for ADC counts.
 * 4. Repeat reads as needed.
 * 5. Call Stop() when sampling is no longer needed.
 * 6. Call Deinitialize() when the peripheral is no longer needed.
 */
class AnalogInput {
 public:
  AnalogInput() = default;
  AnalogInput(const AnalogInput&) = default;
  AnalogInput& operator=(const AnalogInput&) = default;
  AnalogInput(AnalogInput&&) = default;
  AnalogInput& operator=(AnalogInput&&) = default;

  /**
   * @brief Destructor
   */
  virtual ~AnalogInput() = default;

  /**
   * @brief Initializes the analog input peripheral.
   *
   * Configures the ADC hardware (clock, resolution, reference, channel
   * mapping, etc.) according to the implementation-specific settings.
   * Must be called once before any other methods on this interface.
   *
   * @retval ErrorCode::kOk    Peripheral initialized successfully.
   * @retval ErrorCode::kError Initialization failed due to a hardware or
   *                           configuration error.
   */
  virtual ErrorCode Initialize() = 0;

  /**
   * @brief Deinitializes the analog input peripheral.
   *
   * Releases resources allocated by Initialize(). After this call, Start(),
   * Read(), and ReadRaw() must fail until Initialize() is called again.
   * Registers are deinitialized and peripherals can be put in low power or
   * off mode.
   *
   * @retval ErrorCode::kOk Peripheral deinitialized successfully.
   * @retval ErrorCode::kError Deinitialization failed.
   */
  virtual ErrorCode Deinitialize() = 0;

  /**
   * @brief Returns whether the peripheral has been successfully initialized.
   *
   * @retval true  Initialize() has been called and succeeded.
   * @retval false The peripheral is not yet initialized.
   */
  virtual bool IsInitialized() = 0;

  /**
   * @brief Reads normalized converted analog values into @p buffer.
   *
   * Samples are expressed as float in the range from 0.0 to 1.0.
   * Normalization is performed based on the ADC resolution in bits configured.
   * The number of samples read equals the size field of @p buffer.
   * The time this method must wait between elements of @p buffer depend on the
   * implementation and should be specified in its constructor, if needed. To
   * achieve its end, the implementation is free to use a software loop or a DMA
   * transfer, for instance. However, this method must return only when the
   * operation is completed or in case of failure. Calling the Initialize() and
   * Start() methods once is required for this one to run successfully.
   *
   * @param[out] buffer     Destination span that receives the converted
   * samples.
   * @param[in]  timeout_ms Maximum time to wait for all samples, in
   * milliseconds.
   * @retval ErrorCode::kOk      All requested samples were read successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode Read(std::span<float> buffer, uint32_t timeout_ms) = 0;

  /**
   * @brief Reads raw ADC counts into @p buffer.
   *
   * Each element of @p buffer receives raw data without any scaling.
   * The interpretation of the data raw depends on the ADC resolution and
   * reference configuration.
   * The number of samples read equals the size field of @p buffer.
   * The time this method must wait between elements of @p buffer depend on the
   * implementation and should be specified in its constructor, if needed. To
   * achieve its end, the implementation is free to use a software loop or a DMA
   * transfer, for instance. However, this method must return only when the
   * operation is completed or in case of failure.
   * Calling the Initialize() and Start() methods once is required for this one
   * to run successfully.
   *
   * @param[out] buffer     Destination span that receives the raw ADC counts.
   * @param[in]  timeout_ms Maximum time to wait for all samples, in
   * milliseconds.
   * @retval ErrorCode::kOk      All requested samples were read successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode ReadRaw(std::span<uint32_t> buffer,
                            uint32_t timeout_ms) = 0;

  /**
   * @brief Starts analog conversions.
   *
   * Enables the ADC and begins acquiring samples. Must be called before any
   * call to Read() or ReadRaw() for them to run successfully.
   *
   * @param[in] timeout_ms Maximum time to wait for the peripheral to start, in
   * milliseconds.
   * @retval ErrorCode::kOk      Conversions started successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms. Calling the Initialize() method once is required for this one
   * to run successfully.
   */
  virtual ErrorCode Start(uint32_t timeout_ms) = 0;

  /**
   * @brief Stops analog conversions.
   *
   * Halts the ADC. After this call, Read() and ReadRaw() will fail until
   * Start() is called again.
   *
   * @param[in] timeout_ms Maximum time to wait for the peripheral to stop, in
   * milliseconds.
   * @retval ErrorCode::kOk      Conversions stopped successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode Stop(uint32_t timeout_ms) = 0;
};

}  // namespace sfw::hal_interface

#endif  // HAL_INTERFACE_ANALOG_INPUT_HPP