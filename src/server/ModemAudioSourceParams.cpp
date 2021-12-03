//
//  ModemAudioSourceParams.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/3/21.
//

#include "ModemAudioSourceParams.hpp"

ModemParams ModemAudioSourceParams::parseModemParams(
    const std::string& uriParams) {
  SDR::TunerParams params(uriParams);

  if (!params.count("freq")) {
    throw std::runtime_error("Missing required parameter freq");
  }

  {
    const auto freq = params.get<double>("freq");
    if (freq <= 0.0) {
      throw std::runtime_error("Invalid freq value");
    }
    if (freq < 120.0) {
      // Frequency in MHz
      params["freq"] = freq * 1000000.0;
    } else if (freq < 30000.0) {
      // Frequency in kHz
      params["freq"] = freq * 1000.0;
    }
  }

  {
    const auto freq = params.get<double>("freq");
    if (freq >= 65900000.0 && freq <= 108000000.0) {
      params.set("bw", 200000.0);
      params.set("modem", "fm");
      params.set("mono", false);
    } else if (freq >= 162000000.0 && freq <= 163000000.0) {
      params.set("bw", 12500.0);
      params.set("modem", "fm");
      params.set("mono", true);
    } else {
      params.set("bw", 8000.0);
      params.set("modem", "am");
    }
  }

  const auto& modem = params.get<std::string>("modem");
  if (modem == "fm-hd") {
    params.set("program", 0u);
  } else if (modem == "fm") {
    params.set("mono", false);
  } else if (modem == "am") {
    params.set("mode", 0u);
  }

  return params;
}
