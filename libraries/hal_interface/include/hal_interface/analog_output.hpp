// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_INTERFACE_ANALOG_OUTPUT_HPP
#define HAL_INTERFACE_ANALOG_OUTPUT_HPP

#include <cstdint>
#include <span>

#include "hal_interface/error_code.hpp"

namespace sfw::hal_interface {

/**
 * @brief Abstract interface for analog output (DAC) peripherals.
 *
 * Provides methods to start and stop the output stage and to set output
 * values in either normalized floating-point form or raw DAC counts.
 * All the methods on this interface are synchronous / blocking. This means
 * they only return if the operation is finished or if an error occurred.
 *
 * Typical usage:
 * 1. Call Initialize() once to configure the peripheral.
 * 2. Call Start() to begin driving the output.
 * 3. Call Write() for normalized values or WriteRaw() for DAC counts.
 * 4. Repeat writes as needed.
 * 5. Call Stop() when the output is no longer needed.
 * 6. Call Deinitialize() when the peripheral is no longer needed.
 */
class AnalogOutput {
 public:
  AnalogOutput() = default;
  AnalogOutput(const AnalogOutput&) = default;
  AnalogOutput& operator=(const AnalogOutput&) = default;
  AnalogOutput(AnalogOutput&&) = default;
  AnalogOutput& operator=(AnalogOutput&&) = default;

  /**
   * @brief Destructor
   */
  virtual ~AnalogOutput() = default;

  /**
   * @brief Initializes the analog output peripheral.
   *
   * Configures the DAC hardware (clock, resolution, reference, channel
   * mapping, etc.) according to the implementation-specific settings.
   * Must be called once before any other methods on this interface.
   *
   * @retval ErrorCode::kOk    Peripheral initialized successfully.
   * @retval ErrorCode::kError Initialization failed due to a hardware or
   *                           configuration error.
   */
  virtual ErrorCode Initialize() = 0;

  /**
   * @brief Deinitializes the analog output peripheral.
   *
   * Releases resources allocated by Initialize(). After this call, Start(),
   * Write(), and WriteRaw() must fail until Initialize() is called again.
   * Registers are deinitialized and peripherals can be put in low power or
   * off mode.
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
   * @brief Writes floating-point samples from @p buffer to the DAC output.
   *
   * Samples are expressed as float in the range from 0.0 to 1.0. Values outside
   * [0.0, 1.0] are clamped. Normalization should be performed based on the DAC
   * resolution in bits. The number of samples written equals the size field of
   * @p buffer. The time this method must wait between elements of @p buffer
   * depend on the implementation and should be specified in its constructor, if
   * needed. To achieve its end, the implementation is free to use a software
   * loop or a DMA transfer, for instance. However, this method must return only
   * when the operation is completed or in case of failure. Calling the
   * Initialize() and Start() methods once is required for this one to run
   * successfully.
   *
   * @param[in] buffer     Source span containing the samples to output.
   * @param[in] timeout_ms Maximum time to wait for the write to complete, in
   * milliseconds.
   * @retval ErrorCode::kOk      All samples were written successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode Write(std::span<float> buffer, uint32_t timeout_ms) = 0;

  /**
   * @brief Writes raw DAC counts from @p buffer to the DAC output.
   *
   * Each element of @p buffer is passed directly to the DAC hardware.
   * The valid range depends on the DAC resolution.
   * The number of samples written equals the size field of
   * @p buffer. The time this method must wait between elements of @p buffer
   * depend on the implementation and should be specified in its constructor, if
   * needed. To achieve its end, the implementation is free to use a software
   * loop or a DMA transfer, for instance. However, this method must return only
   * when the operation is completed or in case of failure. Calling the
   * Initialize() and Start() methods once is required for this one to run
   * successfully.
   *
   * @param[in] buffer     Source span containing the raw DAC counts to output.
   * @param[in] timeout_ms Maximum time to wait for the write to complete, in
   * milliseconds.
   * @retval ErrorCode::kOk      All samples were written successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode WriteRaw(std::span<uint32_t> buffer,
                             uint32_t timeout_ms) = 0;

  /**
   * @brief Starts the analog output peripheral.
   *
   * Enables the DAC and begins driving the output. Must be called before
   * any call to Write() or WriteRaw() for them to run successfully.
   * Calling the Initialize() method once is required for this one to run
   * successfully.
   *
   * @param[in] timeout_ms Maximum time to wait for the peripheral to start, in
   * milliseconds.
   * @retval ErrorCode::kOk      Output started successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode Start(uint32_t timeout_ms) = 0;

  /**
   * @brief Stops the analog output peripheral.
   *
   * Disables the DAC and ceases driving the output.
   * After this call, Write() or WriteRaw() will fail until Start() is called
   * again. After this call, the output level is implementation-defined until
   * Start() is called again. It is suggested to configure it into a
   * high-impedance state if it is not done automatically.
   *
   * @param[in] timeout_ms Maximum time to wait for the peripheral to stop, in
   * milliseconds.
   * @retval ErrorCode::kOk      Output stopped successfully.
   * @retval ErrorCode::kTimeout The operation did not complete within @p
   * timeout_ms.
   */
  virtual ErrorCode Stop(uint32_t timeout_ms) = 0;
};

}  // namespace sfw::hal_interface

#endif  // HAL_INTERFACE_ANALOG_OUTPUT_HPP