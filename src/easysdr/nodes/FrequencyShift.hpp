//
//  FrequencyShift.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/1/21.
//

#pragma once

#include "easysdr/core/Node.hpp"

#include "Liquid.hpp"

#include <vector>

namespace SDR {

class FrequencyShift final : public Node<
                                 Input<std::vector<std::complex<float>>>,
                                 Output<std::vector<std::complex<float>>>,
                                 Control<double>,
                                 Control<double>> {
 public:
  FrequencyShift(
      double deviceSamplingFreq,
      double sourceFreq,
      double targetFreq)
      : deviceSamplingFreq_(deviceSamplingFreq) {
    setSourceFreq(sourceFreq);
    setTargetFreq(targetFreq);
  }

  enum { IN_INPUT = 0, OUT_OUTPUT, CTRL_SOURCE_FREQ, CTRL_TARGET_FREQ };

  virtual void init() override;
  virtual void process() override;
  virtual void destroy() override;

  double sourceFreq() const;
  void setSourceFreq(double sourceFreq);

  double targetFreq() const;
  void setTargetFreq(double targetFreq);

 private:
  void initShifter(double sourceFreq, double targetFreq);
  void destroyShifter();
  void observeControls();
  void unobserveControls();

 private:
  double deviceSamplingFreq_;
  nco_crcf shifter_ = nullptr;
};

inline double FrequencyShift::sourceFreq() const {
  return portAt<CTRL_SOURCE_FREQ>().value();
}

inline void FrequencyShift::setSourceFreq(double sourceFreq) {
  portAt<CTRL_SOURCE_FREQ>().setValue(sourceFreq);
}

inline double FrequencyShift::targetFreq() const {
  return portAt<CTRL_TARGET_FREQ>().value();
}

inline void FrequencyShift::setTargetFreq(double targetFreq) {
  portAt<CTRL_TARGET_FREQ>().setValue(targetFreq);
}

} // namespace SDR
