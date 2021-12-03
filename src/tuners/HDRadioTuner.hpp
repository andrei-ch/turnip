//
//  HDRadioTuner.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/20/21.
//

#pragma once

#include "TunerWithQueue.hpp"

#include <easysdr/nodes/DecodeNRSC5.hpp>
#include <easysdr/nodes/MP3Encode.hpp>
#include <easysdr/nodes/SDRPlayInput.hpp>

namespace sdrplay {
class device;
}

namespace SDR {

using SDRDevice = sdrplay::device;

struct HDRadioTunerAdvancedParams {
  unsigned int outputBitrateKbps = 128;
};

class HDRadioTuner : public TunerWithQueue<MP3Packet, MetadataPacket> {
 public:
  using SDRPlayInput = SDR::SDRPlayInput<int16_t>;
  using DecodeNRSC5 = SDR::DecodeNRSC5<int16_t>;

  HDRadioTuner(
      SDRDevice* device,
      const TunerParams& params,
      const HDRadioTunerAdvancedParams& advancedParams =
          HDRadioTunerAdvancedParams{});
  HDRadioTuner(
      SDRDevice* device,
      double frequency,
      unsigned int program,
      const HDRadioTunerAdvancedParams& advancedParams =
          HDRadioTunerAdvancedParams{});

  virtual unsigned int audioSamplingRate() const override;
  virtual unsigned int outputBitrateKbps() const override;

 private:
  constexpr static double deviceSamplingRate() {
    return 744187.5;
  }

 private:
  SDRPlayInput sdrInput_;
  DecodeNRSC5 nrsc5Decoder_;
  MP3Encode mp3Encoder_;
  QueueOut<MP3Packet> mp3Output_;
  QueueOut<MetadataPacket> sdrMetadataOutput_;
  QueueOut<MetadataPacket> nrsc5MetadataOutput_;
};

inline unsigned int HDRadioTuner::audioSamplingRate() const {
  return 44100;
}

inline unsigned int HDRadioTuner::outputBitrateKbps() const {
  return mp3Encoder_.bitrateKbps();
}

} // namespace SDR
