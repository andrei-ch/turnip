//
//  SDRPlayInput.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 5/14/21.
//

#include "SDRPlayInput.hpp"

#include <boost/format.hpp>
#include <algorithm>

using namespace std::literals;

namespace SDR {

template <typename T>
void SDRPlayInput<T>::init() {
  if (frequency() < device()->min_center_freq()) {
    throw std::runtime_error("frequency too low for device");
  }
  stream_ = device()->template open_stream<T>();
  device()->add_observer(this);
  device()->start(sampleRate_, frequency(), autoGain_);
  observeControls();
}

template <typename T>
void SDRPlayInput<T>::process() {
  const auto sample_buffer = stream_->read_next_buffer();

  std::vector<T> out;
  out.assign(sample_buffer->samples.begin(), sample_buffer->samples.end());
  this->template setData<OUT_OUTPUT>(std::move(out));

  std::lock_guard<std::mutex> lock(metadata_mutex_);
  if (!metadata_.empty()) {
    this->template setData<OUT_METADATA>(std::move(metadata_));
    metadata_.clear();
  }
}

template <typename T>
void SDRPlayInput<T>::destroy() {
  unobserveControls();
  device()->stop();
  device()->remove_observer(this);
  stream_ = nullptr;
}

template <typename T>
void SDRPlayInput<T>::device_params_changed(
    const sdrplay::device_params& params) {
  std::lock_guard<std::mutex> lock(metadata_mutex_);
  metadata_["sdrplay.gain"] = params.gain;
  metadata_["sdrplay.rf_gr"] = params.rf_gr;
  metadata_["sdrplay.if_gr"] = params.if_gr;
  metadata_["sdrplay.freq"] = params.freq;
  metadata_["sdrplay.lna_state"] = params.lna_state;
  metadata_["sdrplay.num_lna_states"] = device()->num_lna_states(params.freq);
}

template <typename T>
void SDRPlayInput<T>::observeControls() {
  this->template observe<CTRL_FREQ>(
      [this](double freq) { device()->set_center_freq(freq); });
  this->template observe<CTRL_LNA_STATE>(
      [this](unsigned int state) { device()->set_lna_state(state); });
}

template <typename T>
void SDRPlayInput<T>::unobserveControls() {
  this->template unobserve<CTRL_FREQ>();
  this->template unobserve<CTRL_LNA_STATE>();
}

template class SDRPlayInput<uint8_t>;
template class SDRPlayInput<int16_t>;
template class SDRPlayInput<float>;
template class SDRPlayInput<std::complex<float>>;

} // namespace SDR
