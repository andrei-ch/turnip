//
//  FMTuner.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/6/21.
//

#include "FMTuner.hpp"

#include <algorithm>

namespace SDR {

FMTuner::FMTuner(
    SDRDevice* device,
    const TunerParams& params,
    const FMTunerAdvancedParams& advancedParams /*= FMTunerAdvancedParams{}*/)
    : FMTuner(
          device,
          params.get<double>("freq"),
          params.get<double>("bw"),
          params.get<bool>("mono"),
          advancedParams) {}

FMTuner::FMTuner(
    SDRDevice* device,
    double frequency,
    double bandwidth,
    bool mono /*= false*/,
    const FMTunerAdvancedParams& advancedParams /*= FMTunerAdvancedParams{}*/)
    : audioSamplingFreq_(advancedParams.audioSamplingFreq),
      sdrInput_(device, advancedParams.deviceSamplingFreq, frequency),
      iqResample_(advancedParams.deviceSamplingFreq, bandwidth),
      demodFMS_(bandwidth),
      audioResample_(bandwidth, advancedParams.audioSamplingFreq),
      stereoResample_(bandwidth, advancedParams.audioSamplingFreq),
      muxFMS_(advancedParams.audioSamplingFreq),
      mp3Encoder_(
          advancedParams.audioSamplingFreq,
          mono ? 1 : 2,
          advancedParams.outputBitrateKbps),
      mp3Output_(*audioQueue()),
      metadataOutput_(*metadataQueue()) {
  if (frequency < device->min_center_freq()) {
    throw std::runtime_error("frequency too low");
  }

  // Assemble graph
  graph()
      .connectQueued(sdrInput_, iqResample_)
      .connect(iqResample_, demodFM_)
      .connect(demodFM_, audioResample_);

  if (mono) {
    graph().connect(audioResample_, floatToShort_);
  } else {
    graph()
        .connect(demodFM_, demodFMS_)
        .connect(demodFMS_, stereoResample_)
        .connect<AudioResample::OUT_OUTPUT, MuxFMS::IN_STEREO>(
            stereoResample_, muxFMS_)
        .connect<AudioResample::OUT_OUTPUT, MuxFMS::IN_MONO>(
            audioResample_, muxFMS_)
        .connect(muxFMS_, floatToShort_);
  }

  graph()
      .connect(floatToShort_, mp3Encoder_)
      .connect<MP3Encode::OUT_OUTPUT, QueueOut<MP3Packet>::IN_INPUT_VECTOR>(
          mp3Encoder_, mp3Output_)
      .connect<SDRPlayInput::OUT_METADATA, QueueOut<MetadataPacket>::IN_INPUT>(
          sdrInput_, metadataOutput_);

  // Bind controls
  graph()
      .bind<SDRPlayInput::CTRL_FREQ>(sdrInput_, "freq")
      .bind<SDRPlayInput::CTRL_LNA_STATE>(sdrInput_, "lna_state")
      .bind<IQResample::CTRL_TARGET_FREQ>(iqResample_, "bw")
      .bind<AudioResample::CTRL_SOURCE_FREQ>(audioResample_, "bw")
      .bind<AudioResample::CTRL_SOURCE_FREQ>(stereoResample_, "bw");
}

} // namespace SDR
