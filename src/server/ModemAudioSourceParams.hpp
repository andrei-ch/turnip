//
//  ModemAudioSourceParams.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/3/21.
//

#pragma once

#include "tuners/TunerParams.hpp"

namespace sdrplay {
class device;
}

using SDRDevice = sdrplay::device;
using ModemParams = SDR::TunerParams;

class ModemAudioSourceParams {
 public:
  ModemAudioSourceParams(SDRDevice* device, const std::string& uriParams)
      : fDevice(device), fModemParams(parseModemParams(uriParams)) {}

  SDRDevice* device() const {
    return fDevice;
  }

  const std::string& modem() const {
    return boost::get<std::string>(fModemParams.at("modem"));
  }

  const ModemParams& params() const {
    return fModemParams;
  }

 private:
  static ModemParams parseModemParams(const std::string& params);

 private:
  SDRDevice* fDevice;
  const ModemParams fModemParams;
};
