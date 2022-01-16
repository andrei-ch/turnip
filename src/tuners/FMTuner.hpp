//
//  FMTuner.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/6/21.
//

#pragma once

#include "TunerWithQueue.hpp"

#include <easysdr/core/Metadata.hpp>
#include <easysdr/core/QueueOut.hpp>
#include <easysdr/nodes/AudioAutoGain.hpp>
#include <easysdr/nodes/Convert.hpp>
#include <easysdr/nodes/DemodulateFM.hpp>
#include <easysdr/nodes/DemodulateFMS.hpp>
#include <easysdr/nodes/FrequencyShift.hpp>
#include <easysdr/nodes/MP3Encode.hpp>
#include <easysdr/nodes/MuxFMS.hpp>
#include <easysdr/nodes/Resample.hpp>
#include <easysdr/nodes/SDRPlayInput.hpp>

namespace sdrplay {
class device;
}

namespace SDR {

using SDRDevice = sdrplay::device;

struct FMTunerAdvancedParams {
  double deviceSamplingFreq = 2000000.0;
  unsigned int audioSamplingFreq = 44100;
  unsigned int outputBitrateKbps = 128;
};

class FMTuner : public TunerWithQueue<MP3Packet, MetadataPacket> {
 public:
  using SDRPlayInput = SDR::SDRPlayInput<std::complex<float>>;
  using IQResample = SDR::Resample<std::complex<float>>;
  using AudioResample = SDR::Resample<float>;
  using FloatToShort = SDR::Convert<float, short>;

  FMTuner(
      SDRDevice* device,
      const TunerParams& params,
      const FMTunerAdvancedParams& advancedParams = FMTunerAdvancedParams{});

  FMTuner(
      SDRDevice* device,
      double frequency,
      double bandwidth,
      bool mono = false,
      const FMTunerAdvancedParams& advancedParams = FMTunerAdvancedParams{});

  virtual unsigned int audioSamplingRate() const override;
  virtual unsigned int outputBitrateKbps() const override;

 private:
  unsigned int audioSamplingFreq_;
  SDRPlayInput sdrInput_;
  IQResample iqResample_;
  DemodulateFM demodFM_;
  DemodulateFMS demodFMS_;
  AudioResample audioResample_;
  AudioResample stereoResample_;
  MuxFMS muxFMS_;
  AudioAutoGain autoGain_;
  FloatToShort floatToShort_;
  MP3Encode mp3Encoder_;
  QueueOut<MP3Packet> mp3Output_;
  QueueOut<MetadataPacket> metadataOutput_;
};

inline unsigned int FMTuner::audioSamplingRate() const {
  return audioSamplingFreq_;
}

inline unsigned int FMTuner::outputBitrateKbps() const {
  return mp3Encoder_.bitrateKbps();
}

} // namespace SDR
