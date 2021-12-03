//
//  ModemContext.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/30/21.
//

#include "ModemContext.hpp"

#include "tuners/AMTuner.hpp"
#include "tuners/FMTuner.hpp"
#include "tuners/HDRadioTuner.hpp"

SDR::BaseTuner* ModemContext::activeTuner_ = nullptr;

ModemContext::~ModemContext() {
  if (tuner_) {
    if (activeTuner_ == tuner_) {
      activeTuner_ = nullptr;
    }
    tuner_->removeObserver(this);
    tuner_->stopRunning();
  }
}

void ModemContext::ensureNoActiveTuners() {
  assert(!tuner_);
  if (!activeTuner_) {
    return;
  }

  activeTuner_->stopRunning();
  activeTuner_ = nullptr;
}

void ModemContext::onStopped(SDR::BaseTuner* tuner) {
  assert(tuner == tuner_);
  assert(!tunerStopped_);
  tunerStopped_ = true;
}

bool ModemContext::ensureTunerStarted() {
  if (tuner_) {
    return true;
  }

  if (activeTuner_) {
    std::cerr << "Cannot start tuner - another session is already in progress"
              << std::endl;
    return false;
  }

  tunerStopped_ = false;

  try {
    const auto device = modemParams().device();
    const auto& modem = modemParams().modem();
    const auto& initialParams = modemParams().params();

    if (modem == "am") {
      tuner_ = new SDR::AMTuner(
          device,
          initialParams,
          SDR::AMTunerAdvancedParams{.outputBitrateKbps = outputBitrateKbps_});
    } else if (modem == "fm") {
      tuner_ = new SDR::FMTuner(
          device,
          initialParams,
          SDR::FMTunerAdvancedParams{.outputBitrateKbps = outputBitrateKbps_});
    } else if (modem == "fm-hd") {
      tuner_ = new SDR::HDRadioTuner(
          device,
          initialParams,
          SDR::HDRadioTunerAdvancedParams{.outputBitrateKbps =
                                              outputBitrateKbps_});
    } else {
      std::cerr << "Cannot start tuner - invalid modem " << modem << std::endl;
      return false;
    }

    std::cout << "Created tuner " << initialParams << std::endl;

    audioSamplingFrequency_ = tuner_->audioSamplingRate();

    tuner_->startRunning();
  } catch (const std::exception& ex) {
    std::cerr << "Exception starting tuner: " << ex.what() << std::endl;
    tuner_ = nullptr;
    return false;
  }

  tuner_->addObserver(this);
  activeTuner_ = tuner_;
  return true;
}

std::shared_ptr<SDR::MP3Packet> ModemContext::tryFetchAudioPacket() {
  using TunerType = SDR::TunerWithQueue<SDR::MP3Packet, SDR::MetadataPacket>;
  const auto tuner = dynamic_cast<TunerType*>(tuner_);
  return tuner->tryFetchAudioPacket();
}

std::shared_ptr<SDR::MetadataPacket> ModemContext::tryFetchMetadataPacket() {
  using TunerType = SDR::TunerWithQueue<SDR::MP3Packet, SDR::MetadataPacket>;
  const auto tuner = dynamic_cast<TunerType*>(tuner_);
  return tuner->tryFetchMetadataPacket();
}

void ModemContext::queueParamUpdate(const std::string& command) {
  if (!tuner_) {
    return;
  }
  try {
    SDR::TunerParams params(command);
    if (params.empty()) {
      return;
    }
    tuner_->postControlUpdates(params);
  } catch (const std::exception& ex) {
    std::cerr << "Exception parsing command: " << ex.what() << std::endl;
  }
}
