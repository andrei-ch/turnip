//
//  device.hpp
//  sdrplay
//
//  Created by Andrei Chtcherbatchenko on 5/13/21.
//

#pragma once

#include "stream.hpp"

#include <sdrplay_api.h>

#include <array>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_set>

namespace sdrplay {

struct device_params {
  unsigned int rf_gr = 0;
  unsigned int if_gr = 0;
  double gain = 0.0;
  double freq = 0.0;
  unsigned int lna_state = 0;
};

class device_events {
 public:
  virtual void device_params_changed(const device_params& params) {}
};

class device final : public std::enable_shared_from_this<device> {
 public:
  device(const std::shared_ptr<sdrplay_api_DeviceT>& dev) : device_ptr_(dev) {}
  ~device();

  void select();
  void release();

  void start(double sample_rate, double freq, bool agc);
  void start(
      double sample_rate,
      sdrplay_api_Bw_MHzT bw_type,
      sdrplay_api_If_kHzT if_type,
      unsigned int decimation,
      double freq,
      bool agc);
  void stop();

  template <typename T>
  std::shared_ptr<stream<T>> open_stream();

  constexpr double min_center_freq() const;
  constexpr unsigned int num_lna_states(double freq) const;

  void set_center_freq(double freq);
  void set_lna_state(unsigned int lna_state);

  void add_observer(device_events* observer);
  void remove_observer(device_events* observer);
  void notify_observers(std::function<void(device_events*)> lambda);

  auto ptr() const;

 private:
  auto handle() const;
  auto tuner() const;
  auto params() const;
  auto& devParams() const;
  auto& channel() const;
  auto& tunerParams() const;
  auto& ctrlParams() const;

 private:
  friend class device_callbacks;
  void rxa_callback(short* xi, short* xq, unsigned int numSamples, bool reset);
  void rxb_callback(short* xi, short* xq, unsigned int numSamples, bool reset);
  void event_callback(
      sdrplay_api_EventT eventId,
      sdrplay_api_TunerSelectT tunerS,
      sdrplay_api_EventParamsT* params);
  void notify_gain_change(const sdrplay_api_GainCbParamT& gainParams);
  void notify_power_overload(
      const sdrplay_api_PowerOverloadCbParamT& powerOverloadParams);
  void notify_device_removed();
  void notify_rspduo_mode_change(
      const sdrplay_api_RspDuoModeCbParamT& rspDuoModeParams);

 private:
  friend class base_stream;
  void close_stream(base_stream* s);

 private:
  enum device_state { Initialized, Selected, Running };
  std::shared_ptr<sdrplay_api_DeviceT> device_ptr_;
  sdrplay_api_DeviceParamsT* params_ = nullptr;
  sdrplay_api_CallbackFnsT cbfns_;
  device_state state_ = Initialized;
  std::unordered_set<base_stream*> streams_;
  std::mutex streams_mutex_;
  std::unordered_set<device_events*> observers_;
  std::mutex observers_mutex_;
};

template <typename T>
inline std::shared_ptr<stream<T>> device::open_stream() {
  const auto s = std::make_shared<stream<T>>(shared_from_this());
  streams_.insert(s.get());
  return s;
}

inline auto device::ptr() const {
  return device_ptr_.get();
}

inline auto device::handle() const {
  return ptr()->dev;
}

inline auto device::tuner() const {
  return ptr()->tuner;
}

inline auto device::params() const {
  return params_;
}

inline auto& device::devParams() const {
  return *params()->devParams;
}

inline auto& device::channel() const {
  return params()->rxChannelA;
}

inline auto& device::tunerParams() const {
  return channel()->tunerParams;
}

inline auto& device::ctrlParams() const {
  return channel()->ctrlParams;
}

constexpr double device::min_center_freq() const {
  // TODO This is RSP1A specific. Support other devices
  return 1024000.0;
}

constexpr unsigned int device::num_lna_states(double freq) const {
  // TODO These are RSP1A specific bands. Support other devices
  constexpr std::array<std::pair<double, unsigned int>, 3> bands = {
      {{60000000, 7}, {1000000000, 10}, {2000000000, 9}}};
  for (const auto& band : bands) {
    if (freq < band.first) {
      return band.second;
    }
  }
  return 0;
}

} // namespace sdrplay
