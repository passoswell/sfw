// Copyright (c) 2026 sfw contributors. All rights reserved.

#ifndef DSP_IIR_IIR_BIQUAD_HPP
#define DSP_IIR_IIR_BIQUAD_HPP

#include <array>
#include <cstdint>
#include <span>

#include "dsp/interface/filter.hpp"

namespace sfw::dsp::iir {

static const uint8_t kCoefficientsPerStage{5U};
static const uint8_t kStateCountPerStage{2U};

template <typename SampleType, std::size_t kStageCount>
class BiquadCascade final : public sfw::dsp::interface::Filter<SampleType> {
 public:
  /**
   * @brief Construct a new Biquad Cascade object
   *
   * @param coefficients The coefficients of all biquad stages. Coefficients
   * shall be stored in the order:
   *                     @code
   *                     {
   *                       b0_0, b1_0, b2_0, a1_0, a2_0,
   *                       b0_1, b1_1, b2_1, a1_1, a2_1,
   *                       ...
   *                     }
   *                     @endcode
   * @param initial_state Constant input value for which the filter shall
   *                      be initialized in steady state.
   */
  BiquadCascade(
      std::array<SampleType, kStageCount * kCoefficientsPerStage> coefficients,
      SampleType initial_state)
    : coefficients_(coefficients) {
    this->Reset(initial_state);
  }

  /**
   * @brief Destructor
   */
  ~BiquadCascade() override = default;

  BiquadCascade(const BiquadCascade&) = delete;
  BiquadCascade& operator=(const BiquadCascade&) = delete;
  BiquadCascade(BiquadCascade&&) = delete;
  BiquadCascade& operator=(BiquadCascade&&) = delete;

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
  bool Process(std::span<const SampleType> input,
               std::span<SampleType> output) override {
    if (input.size() != output.size()) {
      return false;
    }

    std::span<const SampleType> stage_input{input};
    std::span<SampleType> stage_output{output};

    for (std::size_t stage = 0; stage < kStageCount; ++stage) {
      std::span<SampleType, kCoefficientsPerStage> coefficients =
          std::span<SampleType, kCoefficientsPerStage>{
              coefficients_.data() + stage * kCoefficientsPerStage,
              kCoefficientsPerStage};

      std::span<SampleType, kStateCountPerStage> states =
          std::span<SampleType, kStateCountPerStage>{
              states_.data() + stage * kStateCountPerStage,
              kStateCountPerStage};

      this->ProcessStage(coefficients, states, stage_input, stage_output);

      // For the next stage, the output of the current stage becomes the input
      // and we can reuse the same output buffer.
      stage_input = stage_output;
    }
    return true;
  }

  /**
   * @brief Reset the internal states.
   *
   * Initializes the state variables of every biquad in the cascade so
   * that the filter starts in steady state for a constant input equal to
   * @p initial_state. This avoids the startup transient that would occur
   * if the state variables were simply cleared to zero.
   *
   * The reset is performed stage by stage. The first stage is initialized
   * assuming its input is @p initial_state. Each subsequent stage is then
   * initialized using the steady-state output of the previous stage as
   * its input, ensuring that the entire cascade is internally consistent.
   *
   * After this function returns, processing a constant input equal to
   * @p initial_state produces the corresponding steady-state output
   * immediately, without requiring the filter to settle.
   *
   * @param initial_state Constant input value for which the filter shall
   *                      be initialized in steady state.
   */
  void Reset(SampleType initial_state) override {
    SampleType stage_input = initial_state;

    std::span<const SampleType> coefficients{coefficients_.data(),
                                             coefficients_.size()};
    std::span<SampleType> states{states_.data(), states_.size()};

    for (std::size_t stage = 0U; stage < kStageCount; ++stage) {
      std::span<const SampleType, kCoefficientsPerStage> stage_coefficients =
          std::span<const SampleType, kCoefficientsPerStage>{
              coefficients_.data() + stage * kCoefficientsPerStage,
              kCoefficientsPerStage};

      std::span<SampleType, kStateCountPerStage> stage_states =
          std::span<SampleType, kStateCountPerStage>{
              states_.data() + stage * kStateCountPerStage,
              kStateCountPerStage};

      stage_input = ResetStage(stage_input, stage_coefficients, stage_states);
    }
  }

 private:
  void ProcessStage(
      std::span<const SampleType, kCoefficientsPerStage> coefficients,
      std::span<SampleType, kStateCountPerStage> states,
      std::span<const SampleType> input, std::span<SampleType>& output) {
    for (std::size_t sample = 0; sample < input.size(); ++sample) {
      // Coefficients are in the order: b0, b1, b2, a1, a2
      // States are in the order: s1, s2
      // Implement IIR Transposed Direct Form II code here

      // 1. Calculate output
      output[sample] = input[sample] * coefficients[0] + states[0];

      // 2. Update states
      states[0] = input[sample] * coefficients[1] + states[1]
                  - output[sample] * coefficients[3];
      states[1] =
          input[sample] * coefficients[2] - output[sample] * coefficients[4];
    }
  }

  SampleType ResetStage(
      SampleType initial_state,
      std::span<const SampleType, kCoefficientsPerStage> coefficients,
      std::span<SampleType, kStateCountPerStage> states) {
    SampleType b0 = coefficients[0];  // NOLINT
    SampleType b1 = coefficients[1];  // NOLINT
    SampleType b2 = coefficients[2];  // NOLINT
    SampleType a1 = coefficients[3];  // NOLINT
    SampleType a2 = coefficients[4];  // NOLINT

    SampleType dc_gain =
        (b0 + b1 + b2) / (static_cast<SampleType>(1) + a1 + a2);

    SampleType output = initial_state * dc_gain;

    /*
     * Compute the steady-state values of the internal
     * Transposed Direct Form II delay elements.
     *
     *   y  = b0*x + s1
     *   s1 = b1*x - a1*y + s2
     *   s2 = b2*x - a2*y
     */
    states[1] = (b2 * initial_state) - (a2 * output);

    states[0] = (b1 * initial_state) - (a1 * output) + states[1];

    return output;
  }

  std::array<SampleType, kStageCount * kCoefficientsPerStage> coefficients_{};
  std::array<SampleType, kStageCount * kStateCountPerStage> states_{};
};

}  // namespace sfw::dsp::iir

#endif  // DSP_IIR_IIR_BIQUAD_HPP