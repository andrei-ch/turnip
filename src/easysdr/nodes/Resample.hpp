//
//  Resample.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/2/21.
//

#pragma once

#include "easysdr/core/Node.hpp"

#include "Liquid.hpp"

#include <vector>

namespace SDR {

template <typename T>
class Resample final : public Node<
                           Input<std::vector<T>>,
                           Output<std::vector<T>>,
                           Control<double>,
                           Control<double>> {
 public:
  Resample(double sourceFreq, double targetFreq) {
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

  // Stop-band suppression, dB
  constexpr static float As() {
    return 60.0f;
  }

 private:
  void initResampler(double sourceFreq, double targetFreq);
  void observeControls();
  void unobserveControls();

 private:
  std::unique_ptr<liquid::msresampler<T>> msresamp_;
};

template <typename T>
inline double Resample<T>::sourceFreq() const {
  return this->template portAt<CTRL_SOURCE_FREQ>().value();
}

template <typename T>
inline void Resample<T>::setSourceFreq(double sourceFreq) {
  this->template portAt<CTRL_SOURCE_FREQ>().setValue(sourceFreq);
}

template <typename T>
inline double Resample<T>::targetFreq() const {
  return this->template portAt<CTRL_TARGET_FREQ>().value();
}

template <typename T>
inline void Resample<T>::setTargetFreq(double targetFreq) {
  this->template portAt<CTRL_TARGET_FREQ>().setValue(targetFreq);
}

} // namespace SDR
