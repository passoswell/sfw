// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef HAL_INTERFACE_FILTER_HPP
#define HAL_INTERFACE_FILTER_HPP

#include <span>

namespace sfw::dsp::interface
{

/**
 * @brief Base interface for signal filters.
 *
 * A filter receives an input signal and produces an output signal
 * containing the filtered result. The filter may maintain internal
 * state between calls to process().
 *
 * Examples:
 * - FIR filters
 * - IIR filters
 * - Moving average filters
 * - Kalman filters
 * - Equalizers
 *
 * Typical usage:
 * 1. Construct a filter instance.
 * 2. Call process() with input samples and a destination for output samples.
 *
 * @tparam SampleType Sample type used by the filter.
 */
template<typename SampleType>
class Filter
{
public:
  Filter() = default;
  Filter(const Filter&) = default;
  Filter& operator=(const Filter&) = default;
  Filter(Filter&&) = default;
  Filter& operator=(Filter&&) = default;

  virtual ~Filter() = default;

  /**
   * @brief Filter an input signal.
   *
   * Input and output spans are expected to contain the same number
   * of samples.
   *
   * Implementations shall not modify the input buffer.
   *
   * @param input Input samples.
   * @param output Destination for filtered samples.
   *
   * @retval true Success.
   * @retval false Processing failed.
   */
  virtual bool process(
    std::span<const SampleType> input,
    std::span<SampleType> output) = 0;

  /**
   * @brief Reset all internal filter states.
   *
   * After calling reset(), the filter shall behave as if it
   * had just been constructed.
   */
  virtual void reset() = 0;
};

}  // namespace sfw::dsp::interface

#endif  // HAL_INTERFACE_FILTER_HPP