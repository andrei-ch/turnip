//
//  AMTuner.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/3/21.
//

#include "AMTuner.hpp"

#include <algorithm>

namespace SDR {

namespace {
constexpr double MinAMBandwidth = 1000.0;
constexpr double MaxAMBandwidth = 20000.0;

inline double clampFreqToDeviceMin(double frequency, SDRDevice* device) {
  return std::max(frequency, device->min_center_freq());
}
} // namespace

AMTuner::AMTuner(
    SDRDevice* device,
    const TunerParams& params,
    const AMTunerAdvancedParams& advancedParams /*= AMTunerAdvancedParams{}*/)
    : AMTuner(
          device,
          params.get<double>("freq"),
          params.get<double>("bw"),
          params.get<unsigned int>("mode"),
          advancedParams) {}

AMTuner::AMTuner(
    SDRDevice* device,
    double frequency,
    double bandwidth,
    unsigned int mode,
    const AMTunerAdvancedParams& advancedParams /*= AMTunerAdvancedParams{}*/)
    : audioSamplingRate_(advancedParams.audioSamplingFreq),
      sdrInput_(
          device,
          advancedParams.deviceSamplingFreq,
          clampFreqToDeviceMin(frequency, device)),
      freqShift_(
          advancedParams.deviceSamplingFreq,
          clampFreqToDeviceMin(frequency, device),
          frequency),
      iqResample_(advancedParams.deviceSamplingFreq, bandwidth),
      demodAM_(mode),
      audioResample_(bandwidth, advancedParams.audioSamplingFreq),
      mp3Encoder_(
          advancedParams.audioSamplingFreq,
          1,
          advancedParams.outputBitrateKbps),
      mp3Output_(*audioQueue()),
      metadataOutput_(*metadataQueue()) {
  // Assemble graph
  graph()
      .connectQueued(sdrInput_, freqShift_)
      .connect(freqShift_, iqResample_)
      .connect(iqResample_, demodAM_)
      .connect(demodAM_, autoGain_)
      .connect(autoGain_, audioResample_)
      .connect(audioResample_, floatToShort_)
      .connect(floatToShort_, mp3Encoder_)
      .connect<MP3Encode::OUT_OUTPUT, QueueOut<MP3Packet>::IN_INPUT_VECTOR>(
          mp3Encoder_, mp3Output_)
      .connect<SDRPlayInput::OUT_METADATA, QueueOut<MetadataPacket>::IN_INPUT>(
          sdrInput_, metadataOutput_);

  // Validators
  const Graph::BindingValidator<double> centerFreqValidator =
      [device](auto frequency) {
        return clampFreqToDeviceMin(frequency, device);
      };

  const Graph::BindingValidator<double> bandwidthValidator =
      [device](auto bandwidth) {
        return std::clamp(bandwidth, MinAMBandwidth, MaxAMBandwidth);
      };

  // Bind controls
  graph()
      .bind<SDRPlayInput::CTRL_LNA_STATE>(sdrInput_, "lna_state")
      .bind<SDRPlayInput::CTRL_FREQ>(sdrInput_, "freq", centerFreqValidator)
      .bind<FrequencyShift::CTRL_SOURCE_FREQ>(
          freqShift_, "freq", centerFreqValidator)
      .bind<FrequencyShift::CTRL_TARGET_FREQ>(freqShift_, "freq")
      .bind<IQResample::CTRL_TARGET_FREQ>(iqResample_, "bw", bandwidthValidator)
      .bind<AudioResample::CTRL_SOURCE_FREQ>(
          audioResample_, "bw", bandwidthValidator)
      .bind<DemodulateAM::CTRL_MODE>(demodAM_, "mode");
}

} // namespace SDR
