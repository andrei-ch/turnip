//
//  FrequencyShift.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/1/21.
//

#include "FrequencyShift.hpp"

#include <math.h>

namespace SDR {

void FrequencyShift::init() {
  initShifter(sourceFreq(), targetFreq());
  observeControls();
}

void FrequencyShift::process() {
  auto& inData = getData<IN_INPUT>();
  const auto inSize = inData.size();

  if (!shifter_) {
    setData<OUT_OUTPUT>(std::move(inData));
    return;
  }

  std::vector<std::complex<float>> outData;
  outData.resize(inSize);

  if (targetFreq() < sourceFreq()) {
    nco_crcf_mix_block_up(
        shifter_,
        inData.data(),
        outData.data(),
        static_cast<unsigned int>(inSize));
  } else {
    nco_crcf_mix_block_down(
        shifter_,
        inData.data(),
        outData.data(),
        static_cast<unsigned int>(inSize));
  }
  setData<OUT_OUTPUT>(std::move(outData));
}

void FrequencyShift::destroy() {
  unobserveControls();
  destroyShifter();
}

void FrequencyShift::initShifter(double sourceFreq, double targetFreq) {
  if (sourceFreq == targetFreq) {
    destroyShifter();
    return;
  }
  if (!shifter_) {
    shifter_ = nco_crcf_create(LIQUID_VCO);
  }
  nco_crcf_set_frequency(
      shifter_,
      (2.0 * M_PI) * (std::abs(targetFreq - sourceFreq) / deviceSamplingFreq_));
}

void FrequencyShift::destroyShifter() {
  if (shifter_) {
    nco_crcf_destroy(shifter_);
    shifter_ = nullptr;
  }
}

void FrequencyShift::observeControls() {
  observe<CTRL_SOURCE_FREQ>(
      [this](double sourceFreq) { initShifter(sourceFreq, targetFreq()); });
  observe<CTRL_TARGET_FREQ>(
      [this](double targetFreq) { initShifter(sourceFreq(), targetFreq); });
}

void FrequencyShift::unobserveControls() {
  unobserve<CTRL_SOURCE_FREQ>();
  unobserve<CTRL_TARGET_FREQ>();
}

} // namespace SDR
