// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef DSP_FIR_FIR_TRANSPOSED_FORM_HPP
#define DSP_FIR_FIR_TRANSPOSED_FORM_HPP

#include <array>
#include <cstddef>
#include <span>

#include "dsp/interface/filter.hpp"

namespace sfw::dsp::fir {

/**
 * @brief Transposed form FIR filter using a circular sample buffer.
 *
 * Implements:
 *
 * y[n] = h[0]x[n] + h[1]x[n-1] + ... + h[N-1]x[n-(N-1)]
 *
 * The transposed realization stores N - 1 internal state variables
 * corresponding to the delays of the transposed filter structure,
 * requiring less state memory than a direct-form implementation that
 * stores previous input samples.
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
 * 1. Construct a TransposedForm instance with FIR coefficients.
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
class TransposedForm final : public sfw::dsp::interface::Filter<SampleType> {
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
  explicit constexpr TransposedForm(
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
        output[sample] =
            coefficients_.at(kTapCount - 1) * input[sample] + states_.at(0);

        for (std::size_t tap = 0; tap < kTapCount - 2; ++tap) {
          states_.at(tap) =
              coefficients_.at(kTapCount - 2 - tap) * input[sample]
              + states_.at(tap + 1);
        }

        states_.at(kTapCount - 2) = coefficients_.at(0) * input[sample];
      } else {
        output[sample] = coefficients_.at(0) * input[sample] + states_.at(0);

        for (std::size_t tap = 0; tap < kTapCount - 2; ++tap) {
          states_.at(tap) =
              coefficients_.at(tap + 1) * input[sample] + states_.at(tap + 1);
        }

        states_.at(kTapCount - 2) =
            coefficients_.at(kTapCount - 1) * input[sample];
      }
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
    auto accumulator = SampleType{};

    if constexpr (kReverseCoefficients) {
      for (std::size_t tap = 0; tap < kTapCount - 1; ++tap) {
        accumulator += coefficients_.at(tap) * initial_state;

        states_.at(kTapCount - 2 - tap) = accumulator;
      }
    } else {
      for (std::size_t tap = kTapCount - 1; tap > 0; --tap) {
        accumulator += coefficients_.at(tap) * initial_state;

        states_.at(tap - 1) = accumulator;
      }
    }
  }

 private:
  std::array<SampleType, kTapCount> coefficients_;
  std::array<SampleType, kTapCount - 1> states_;
};

}  // namespace sfw::dsp::fir

#endif  // DSP_FIR_FIR_TRANSPOSED_FORM_HPP