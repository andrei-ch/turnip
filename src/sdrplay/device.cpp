//
//  device.cpp
//  sdrplay
//
//  Created by Andrei Chtcherbatchenko on 5/13/21.
//

#include "device.hpp"

#include "api.hpp"

#include <iostream>

namespace sdrplay {

class device_callbacks {
 public:
  device_callbacks() = delete;

  static void rxa_callback(
      short* xi,
      short* xq,
      sdrplay_api_StreamCbParamsT* params,
      unsigned int numSamples,
      unsigned int reset,
      void* cbContext);
  static void rxb_callback(
      short* xi,
      short* xq,
      sdrplay_api_StreamCbParamsT* params,
      unsigned int numSamples,
      unsigned int reset,
      void* cbContext);
  static void event_callback(
      sdrplay_api_EventT eventId,
      sdrplay_api_TunerSelectT tunerS,
      sdrplay_api_EventParamsT* params,
      void* cbContext);
};

namespace {

constexpr double input_params_for_output_rate(
    double output_sample_rate,
    unsigned int* decimation,
    sdrplay_api_If_kHzT* if_type) {
  struct Preset {
    double output_sample_rate;
    sdrplay_api_If_kHzT if_type;
    unsigned decimation;
    double input_sample_rate;
  };

  constexpr std::array<Preset, 12> presets = {{
      {62500, sdrplay_api_IF_1_620, 32, 6000000},
      {125000, sdrplay_api_IF_1_620, 16, 6000000},
      {250000, sdrplay_api_IF_1_620, 8, 6000000},
      {500000, sdrplay_api_IF_1_620, 4, 6000000},
      {1000000, sdrplay_api_IF_1_620, 2, 6000000},
      {2000000, sdrplay_api_IF_1_620, 1, 6000000},
      {96000, sdrplay_api_IF_Zero, 32, 3072000},
      {192000, sdrplay_api_IF_Zero, 16, 3072000},
      {384000, sdrplay_api_IF_Zero, 8, 3072000},
      {768000, sdrplay_api_IF_Zero, 4, 3072000},
      // HD Radio presets
      {1488375, sdrplay_api_IF_Zero, 2, 2976750},
      {744187.5, sdrplay_api_IF_Zero, 4, 2976750},
  }};

  for (const auto& preset : presets) {
    if (output_sample_rate == preset.output_sample_rate) {
      *decimation = preset.decimation;
      *if_type = preset.if_type;
      return preset.input_sample_rate;
    }
  }
  if (output_sample_rate < 2000000) {
    throw std::runtime_error("unsupported sample rate");
  }
  *decimation = 1;
  *if_type = sdrplay_api_IF_Zero;
  return output_sample_rate;
}

constexpr sdrplay_api_Bw_MHzT bw_for_output_rate(double output_sample_rate) {
  constexpr std::array<std::pair<double, sdrplay_api_Bw_MHzT>, 7> bands = {{
      {300000, sdrplay_api_BW_0_200},
      {600000, sdrplay_api_BW_0_300},
      {1536000, sdrplay_api_BW_0_600},
      {5000000, sdrplay_api_BW_1_536},
      {6000000, sdrplay_api_BW_5_000},
      {7000000, sdrplay_api_BW_6_000},
      {8000000, sdrplay_api_BW_7_000},
  }};
  for (const auto& band : bands) {
    if (output_sample_rate < band.first) {
      return band.second;
    }
  }
  return sdrplay_api_BW_8_000;
}

} // namespace

device::~device() {
  release();
}

void device::select() {
  if (state_ != Initialized) {
    return;
  }
  api::select(this);
  if (sdrplay_api_GetDeviceParams(handle(), &params_) != sdrplay_api_Success) {
    api::release(this);
    params_ = nullptr;
    throw std::runtime_error("sdrplay_api_GetDeviceParams");
  }
  state_ = Selected;
}

void device::release() {
  stop();
  if (state_ == Selected) {
    api::release(this);
    params_ = nullptr;
    state_ = Initialized;
  }
}

void device::start(double sample_rate, double freq, bool agc) {
  unsigned int decimation;
  sdrplay_api_If_kHzT if_type;
  const double input_rate =
      input_params_for_output_rate(sample_rate, &decimation, &if_type);
  const sdrplay_api_Bw_MHzT bw_type = bw_for_output_rate(sample_rate);

  start(input_rate, bw_type, if_type, decimation, freq, agc);
}

void device::start(
    double sample_rate,
    sdrplay_api_Bw_MHzT bw_type,
    sdrplay_api_If_kHzT if_type,
    unsigned int decimation,
    double freq,
    bool agc) {
  assert(state_ == Selected);

  auto& devParams = this->devParams();
#ifdef __APPLE__
  // Note: SDRPlay isochronous mode is NOT reliable on macOS Big Sur!
  devParams.mode = sdrplay_api_BULK;
#endif
  devParams.ppm = 0.0;
  devParams.fsFreq.fsHz = sample_rate;
  devParams.rsp1aParams.rfNotchEnable = 0;
  devParams.rsp1aParams.rfDabNotchEnable = 1;

  auto& tunerParams = this->tunerParams();
  tunerParams.bwType = bw_type;
  tunerParams.ifType = if_type;
  tunerParams.gain.gRdB = 50;
  tunerParams.gain.LNAstate = 4;
  tunerParams.rfFreq.rfHz = freq;

  auto& ctrlParams = this->ctrlParams();
  ctrlParams.dcOffset.DCenable = 1;
  ctrlParams.dcOffset.IQenable = 1;
  ctrlParams.decimation.enable = decimation != 1 ? 1 : 0;
  ctrlParams.decimation.decimationFactor = decimation;
  ctrlParams.decimation.wideBandSignal = if_type == sdrplay_api_IF_Zero ? 1 : 0;

  if (agc) {
    ctrlParams.agc.enable = sdrplay_api_AGC_CTRL_EN;
    ctrlParams.agc.setPoint_dBfs = -20;
    ctrlParams.agc.attack_ms = 500;
    ctrlParams.agc.decay_ms = 500;
    ctrlParams.agc.decay_delay_ms = 200;
    ctrlParams.agc.decay_threshold_dB = 5;
    ctrlParams.agc.syncUpdate = 0;
  } else {
    ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;
  }

  cbfns_.StreamACbFn = device_callbacks::rxa_callback;
  cbfns_.StreamBCbFn = device_callbacks::rxb_callback;
  cbfns_.EventCbFn = device_callbacks::event_callback;

  if (sdrplay_api_Init(handle(), &cbfns_, this) != sdrplay_api_Success) {
    throw std::runtime_error("sdrplay_api_Init");
  }
  state_ = Running;
}

void device::stop() {
  if (state_ == Running) {
    sdrplay_api_Uninit(handle());
    state_ = Selected;
  }
}

void device::close_stream(base_stream* s) {
  std::lock_guard<std::mutex> lock(streams_mutex_);
  const auto found = streams_.find(s);
  if (found == streams_.end()) {
    throw std::runtime_error("stream not open");
  }
  streams_.erase(s);
}

void device::add_observer(device_events* observer) {
  std::lock_guard<std::mutex> lock(observers_mutex_);
  if (observers_.find(observer) != observers_.end()) {
    throw std::runtime_error("observer already added");
  }
  observers_.insert(observer);
}

void device::remove_observer(device_events* observer) {
  std::lock_guard<std::mutex> lock(observers_mutex_);
  if (observers_.find(observer) == observers_.end()) {
    throw std::runtime_error("observer not added");
  }
  observers_.erase(observer);
}

void device::notify_observers(std::function<void(device_events*)> lambda) {
  std::lock_guard<std::mutex> lock(observers_mutex_);
  for (const auto& observer : observers_) {
    lambda(observer);
  }
}

void device::set_center_freq(double freq) {
  tunerParams().rfFreq.rfHz = freq;
  const auto err = sdrplay_api_Update(
      handle(),
      tuner(),
      sdrplay_api_Update_Tuner_Frf,
      sdrplay_api_Update_Ext1_None);
  if (err != sdrplay_api_Success) {
    throw std::runtime_error("sdrplay_api_Update");
  }
}

void device::set_lna_state(unsigned int lna_state) {
  tunerParams().gain.LNAstate = static_cast<unsigned char>(lna_state);
  const auto err = sdrplay_api_Update(
      handle(),
      tuner(),
      sdrplay_api_Update_Tuner_Gr,
      sdrplay_api_Update_Ext1_None);
  if (err != sdrplay_api_Success) {
    throw std::runtime_error("sdrplay_api_Update");
  }
}

void device::rxa_callback(
    short* xi,
    short* xq,
    unsigned int num_samples,
    bool reset) {
  std::lock_guard<std::mutex> lock(streams_mutex_);
  if (reset) {
    for (const auto& stream : streams_) {
      stream->reset();
    }
  }
  for (const auto& stream : streams_) {
    stream->process_data(xi, xq, static_cast<size_t>(num_samples));
  }
}

void device::rxb_callback(
    short* xi,
    short* xq,
    unsigned int num_samples,
    bool reset) {
  throw std::runtime_error("not implemented");
}

void device::event_callback(
    sdrplay_api_EventT eventId,
    sdrplay_api_TunerSelectT tunerS,
    sdrplay_api_EventParamsT* params) {
  switch (eventId) {
    case sdrplay_api_GainChange:
      notify_gain_change(params->gainParams);
      break;
    case sdrplay_api_PowerOverloadChange:
      sdrplay_api_Update(
          handle(),
          tuner(),
          sdrplay_api_Update_Ctrl_OverloadMsgAck,
          sdrplay_api_Update_Ext1_None);
      notify_power_overload(params->powerOverloadParams);
      break;
    case sdrplay_api_DeviceRemoved:
      notify_device_removed();
      break;
    case sdrplay_api_RspDuoModeChange:
      notify_rspduo_mode_change(params->rspDuoModeParams);
      break;
  }
}

void device::notify_gain_change(const sdrplay_api_GainCbParamT& gainParams) {
  const device_params params = {.rf_gr = gainParams.lnaGRdB,
                                .if_gr = gainParams.gRdB,
                                .gain = gainParams.currGain,
                                .freq = tunerParams().rfFreq.rfHz,
                                .lna_state = tunerParams().gain.LNAstate};
  notify_observers([&](auto obs) { obs->device_params_changed(params); });
}

void device::notify_power_overload(
    const sdrplay_api_PowerOverloadCbParamT& powerOverloadParams) {
  switch (powerOverloadParams.powerOverloadChangeType) {
    case sdrplay_api_Overload_Detected:
      // TODO report to app through metadata channels
      std::cout << "ADC overload detected" << std::endl;
      break;
    case sdrplay_api_Overload_Corrected:
      // TODO report to app through metadata channels
      std::cout << "ADC overload corrected" << std::endl;
      break;
  }
}

void device::notify_device_removed() {
  std::cout << "Device removed" << std::endl;
}

void device::notify_rspduo_mode_change(
    const sdrplay_api_RspDuoModeCbParamT& rspDuoModeParams) {
  std::cout << "RSPDuo mode changed" << std::endl;
}

void device_callbacks::rxa_callback(
    short* xi,
    short* xq,
    sdrplay_api_StreamCbParamsT* params,
    unsigned int numSamples,
    unsigned int reset,
    void* cbContext) {
  const auto p_this = reinterpret_cast<device*>(cbContext);
  p_this->rxa_callback(xi, xq, numSamples, !!reset);
}

void device_callbacks::rxb_callback(
    short* xi,
    short* xq,
    sdrplay_api_StreamCbParamsT* params,
    unsigned int numSamples,
    unsigned int reset,
    void* cbContext) {
  const auto p_this = reinterpret_cast<device*>(cbContext);
  p_this->rxb_callback(xi, xq, numSamples, !!reset);
}

void device_callbacks::event_callback(
    sdrplay_api_EventT eventId,
    sdrplay_api_TunerSelectT tunerS,
    sdrplay_api_EventParamsT* params,
    void* cbContext) {
  const auto p_this = reinterpret_cast<device*>(cbContext);
  p_this->event_callback(eventId, tunerS, params);
}

} // namespace sdrplay
