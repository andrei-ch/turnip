//
//  AMTuner.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/3/21.
//

#pragma once

#include "TunerWithQueue.hpp"

#include <easysdr/core/Metadata.hpp>
#include <easysdr/core/QueueOut.hpp>
#include <easysdr/nodes/AudioAutoGain.hpp>
#include <easysdr/nodes/Convert.hpp>
#include <easysdr/nodes/DemodulateAM.hpp>
#include <easysdr/nodes/FrequencyShift.hpp>
#include <easysdr/nodes/MP3Encode.hpp>
#include <easysdr/nodes/Resample.hpp>
#include <easysdr/nodes/SDRPlayInput.hpp>

namespace sdrplay {
class device;
}

namespace SDR {

using SDRDevice = sdrplay::device;

struct AMTunerAdvancedParams {
  double deviceSamplingFreq = 2000000.0;
  unsigned int audioSamplingFreq = 48000;
  unsigned int outputBitrateKbps = 128;
};

class AMTuner : public TunerWithQueue<MP3Packet, MetadataPacket> {
 public:
  using SDRPlayInput = SDR::SDRPlayInput<std::complex<float>>;
  using IQResample = SDR::Resample<std::complex<float>>;
  using AudioResample = SDR::Resample<float>;
  using FloatToShort = SDR::Convert<float, short>;

  AMTuner(
      SDRDevice* device,
      const TunerParams& params,
      const AMTunerAdvancedParams& advancedParams = AMTunerAdvancedParams{});

  AMTuner(
      SDRDevice* device,
      double frequency,
      double bandwidth,
      unsigned int mode,
      const AMTunerAdvancedParams& advancedParams = AMTunerAdvancedParams{});

  virtual unsigned int audioSamplingRate() const override;
  virtual unsigned int outputBitrateKbps() const override;

 private:
  unsigned int audioSamplingRate_;
  SDRPlayInput sdrInput_;
  FrequencyShift freqShift_;
  IQResample iqResample_;
  DemodulateAM demodAM_;
  AudioResample audioResample_;
  AudioAutoGain autoGain_;
  FloatToShort floatToShort_;
  MP3Encode mp3Encoder_;
  QueueOut<MP3Packet> mp3Output_;
  QueueOut<MetadataPacket> metadataOutput_;
};

inline unsigned int AMTuner::audioSamplingRate() const {
  return audioSamplingRate_;
}

inline unsigned int AMTuner::outputBitrateKbps() const {
  return mp3Encoder_.bitrateKbps();
}

} // namespace SDR
