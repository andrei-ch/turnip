//
//  SDRPlayInput.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 5/14/21.
//

#pragma once

#include "easysdr/core/Metadata.hpp"
#include "easysdr/core/Node.hpp"

#include <sdrplay/device.hpp>

#include <chrono>
#include <memory>
#include <vector>

namespace SDR {

template <typename T>
class SDRPlayInput final : public Node<
                               Output<std::vector<T>>,
                               Output<MetadataPacket>,
                               Control<double>,
                               Control<unsigned int>>,
                           public sdrplay::device_events {
 public:
  SDRPlayInput(
      sdrplay::device* device,
      double sampleRate,
      double frequency,
      bool autoGain = true)
      : device_(device), sampleRate_(sampleRate), autoGain_(autoGain) {
    setFrequency(frequency);
  }

  enum { OUT_OUTPUT = 0, OUT_METADATA, CTRL_FREQ, CTRL_LNA_STATE };

  virtual void init() override;
  virtual void process() override;
  virtual void destroy() override;

  double frequency() const;
  void setFrequency(double frequency);

  sdrplay::device* device() const {
    return device_;
  }

  // sdrplay::device_events
  virtual void device_params_changed(
      const sdrplay::device_params& params) override;

 private:
  void observeControls();
  void unobserveControls();

 private:
  sdrplay::device* device_;
  std::shared_ptr<sdrplay::stream<T>> stream_;
  double sampleRate_;
  bool autoGain_;
  std::mutex metadata_mutex_;
  MetadataPacket metadata_;
};

template <typename T>
inline double SDRPlayInput<T>::frequency() const {
  return this->template portAt<CTRL_FREQ>().value();
}

template <typename T>
inline void SDRPlayInput<T>::setFrequency(double frequency) {
  this->template portAt<CTRL_FREQ>().setValue(frequency);
}

} // namespace SDR
