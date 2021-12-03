//
//  ModemContext.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/30/21.
//

#pragma once

#include "ModemAudioSourceParams.hpp"

#include "tuners/TunerWithQueue.hpp"

#include <easysdr/core/Metadata.hpp>
#include <easysdr/nodes/MP3Encode.hpp>

class ModemContext : public SDR::TunerEvents {
 public:
  ModemContext(const ModemAudioSourceParams& modemParams)
      : modemParams_(modemParams) {}
  ~ModemContext();

  const ModemAudioSourceParams& modemParams() const {
    return modemParams_;
  }

  unsigned int audioSamplingFrequency() const {
    return audioSamplingFrequency_;
  }

  unsigned int outputBitrateKbps() const {
    return tuner_ ? tuner_->outputBitrateKbps() : outputBitrateKbps_;
  }

  SDR::BaseTuner* tuner() const {
    return tuner_;
  }

  bool isTunerStopped() const {
    return tunerStopped_;
  }

  bool isTunerActive() const {
    return tuner_ && tuner_ == activeTuner_;
  }

  void ensureNoActiveTuners();
  bool ensureTunerStarted();

  std::shared_ptr<SDR::MP3Packet> tryFetchAudioPacket();
  std::shared_ptr<SDR::MetadataPacket> tryFetchMetadataPacket();

  void queueParamUpdate(const std::string& command);

 protected:
  virtual void onStopped(SDR::BaseTuner* tuner) override;

 private:
  const ModemAudioSourceParams modemParams_;
  static SDR::BaseTuner* activeTuner_;
  SDR::BaseTuner* tuner_ = nullptr;
  bool tunerStopped_ = false;
  unsigned int audioSamplingFrequency_ = 0;
  unsigned int outputBitrateKbps_ = 128;
};
