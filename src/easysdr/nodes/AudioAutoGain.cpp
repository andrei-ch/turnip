//
//  AudioAutoGain.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/2/21.
//

#include "AudioAutoGain.hpp"

namespace SDR {

void AudioAutoGain::init() {
  gain_ = 1.f;
}

void AudioAutoGain::process() {
  auto& inData = getData<IN_INPUT>();

  float newGain = gain_;

  float peak = 0.f;
  for (float sample : inData) {
    peak = std::max(peak, std::abs(sample * gain_));
  }

  if (peak > 0.f) {
    constexpr float good_zone_min = .1f;
    constexpr float good_zone_max = .2f;
    constexpr float target = (good_zone_min + good_zone_max) / 2.f;
    constexpr float max_inc_rate = 1.05f;
    constexpr float max_dec_rate = .5f;

    if (peak < good_zone_min) {
      newGain = gain_ * std::min(target / peak, max_inc_rate);
    } else if (peak > good_zone_max) {
      newGain = gain_ * std::max(target / peak, max_dec_rate);
    }
  }

  std::vector<float> outData;
  outData.resize(inData.size());

  for (size_t idx = 0; idx < inData.size(); ++idx) {
    const float a = static_cast<float>(idx) / static_cast<float>(inData.size());
    const float g = newGain * a + (gain_ * (1.f - a));
    outData[idx] = inData[idx] * g;
  }

  gain_ = newGain;

  setData<OUT_OUTPUT>(std::move(outData));
}

} // namespace SDR
