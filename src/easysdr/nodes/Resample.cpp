//
//  Resample.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/2/21.
//

#include "Resample.hpp"

namespace SDR {

template <typename T>
void Resample<T>::init() {
  initResampler(sourceFreq(), targetFreq());
  observeControls();
}

template <typename T>
void Resample<T>::process() {
  auto& inData = this->template getData<IN_INPUT>();
  const auto inSize = inData.size();

  std::vector<T> outData;
  outData.resize(msresamp_->estimate(inSize));

  const auto outSize =
      msresamp_->execute(inSize, inData.data(), outData.data());

  outData.resize(outSize);

  this->template setData<OUT_OUTPUT>(std::move(outData));
}

template <typename T>
void Resample<T>::destroy() {
  unobserveControls();
  msresamp_.reset();
}

template <typename T>
void Resample<T>::initResampler(double sourceFreq, double targetFreq) {
  msresamp_ =
      std::make_unique<liquid::msresampler<T>>(targetFreq / sourceFreq, As());
}

template <typename T>
void Resample<T>::observeControls() {
  this->template observe<CTRL_SOURCE_FREQ>(
      [this](double sourceFreq) { initResampler(sourceFreq, targetFreq()); });
  this->template observe<CTRL_TARGET_FREQ>(
      [this](double targetFreq) { initResampler(sourceFreq(), targetFreq); });
}

template <typename T>
void Resample<T>::unobserveControls() {
  this->template unobserve<CTRL_SOURCE_FREQ>();
  this->template unobserve<CTRL_TARGET_FREQ>();
}

template class Resample<float>;
template class Resample<std::complex<float>>;

} // namespace SDR
