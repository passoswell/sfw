// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef DSP_FIR_FIR_DIRECT_FORM_HPP
#define DSP_FIR_FIR_DIRECT_FORM_HPP

#include <array>
#include <cstddef>
#include <span>

#include "dsp/interface/filter.hpp"

namespace sfw::dsp::fir {

/**
 * @brief Direct Form FIR filter using a circular sample buffer.
 *
 * Implements:
 *
 * y[n] = h[0]x[n] + h[1]x[n-1] + ... + h[N-1]x[n-(N-1)]
 *
 * If kReverseCoefficients is false, coefficients must be stored as
 * h[0], h[1], ..., h[kTapCount - 1]
 *
 * If kReverseCoefficients is true, coefficients must be stored as
 * h[kTapCount - 1], ..., h[1], h[0]
 *
 * If done differently, the filter may compute incorrect results.
 *
 * Typical usage:
 * 1. Construct a DirectForm instance with FIR coefficients.
 * 2. Call Process() with input samples and a destination for output samples.
 * 3. Call Reset() to clear filter state if needed.
 *
 * @tparam SampleType Sample type.
 * @tparam kTapCount Number of FIR coefficients and FIR states.
 * @tparam kReverseCoefficients
 *         false: coefficients are stored as
 *                h[0], h[1], ..., h[kTapCount - 1]
 *
 *         true:  coefficients are stored as
 *                h[kTapCount - 1], ..., h[1], h[0]
 */
template <typename SampleType, std::size_t kTapCount,
          bool kReverseCoefficients = false>
class DirectForm final : public sfw::dsp::interface::Filter<SampleType> {
 public:
  /**
   * @brief Construct a FIR filter.
   *
   * If kReverseCoefficients is false, coefficients must be stored as
   * h[0], h[1], ..., h[kTapCount - 1]
   *
   * If kReverseCoefficients is true, coefficients must be stored as
   * h[kTapCount - 1], ..., h[1], h[0]
   *
   * If done differently, the filter may compute incorrect results.
   *
   * The @p initial_state is intended to be used to avoid transients on the
   * initial filter response. The method should then compute the internal states
   * values to match it.
   *
   * @param coefficients FIR coefficients.
   */
  explicit constexpr DirectForm(
      const std::array<SampleType, kTapCount>& coefficients,
      SampleType initial_state)
    : coefficients_(coefficients) {
    this->Reset(initial_state);
  }

  /**
   * @brief Process a block of samples.
   *
   * @param input Input samples.
   * @param output Output samples.
   *
   * @return true if processing succeeded.
   * @return false if the span sizes differ.
   */
  bool Process(std::span<const SampleType> input,
               std::span<SampleType> output) override {
    if (input.size() != output.size()) {
      return false;
    }

    for (std::size_t sample = 0; sample < input.size(); ++sample) {
      if constexpr (kReverseCoefficients) {
        ++head_;
        if (head_ >= kTapCount) {
          head_ = 0;
        }
      } else {
        if (head_ == 0) {
          head_ = kTapCount;
        }
        --head_;
      }

      samples_buffer_.at(head_) = input[sample];
      std::size_t sample_index = head_;
      SampleType accumulator{};

      for (std::size_t tap = 0; tap < kTapCount; ++tap) {
        accumulator += coefficients_.at(tap) * samples_buffer_.at(sample_index);

        ++sample_index;

        if (sample_index == kTapCount) {
          sample_index = 0;
        }
      }

      output[sample] = accumulator;
    }

    return true;
  }

  /**
   * @brief Reset filter state.
   *
   * After calling reset(), the filter shall behave as if it had just been
   * constructed.
   * The @p initial_state is intended to be used to avoid transients on the
   * initial filter response. The method should then compute the internal states
   * values to match it.
   *
   * @param initial_state The value to initialize all internal states.
   */
  void Reset(SampleType initial_state) override {
    samples_buffer_.fill(initial_state);
    if constexpr (kReverseCoefficients) {
      head_ = kTapCount - 1;
    } else {
      head_ = 0;
    }
  }

 private:
  std::array<SampleType, kTapCount> coefficients_;
  std::array<SampleType, kTapCount> samples_buffer_;
  std::size_t head_{};
};

}  // namespace sfw::dsp::fir

#endif  // DSP_FIR_FIR_DIRECT_FORM_HPP