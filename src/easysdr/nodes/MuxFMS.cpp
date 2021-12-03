//
//  MuxFMS.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/6/21.
//

#include "MuxFMS.hpp"

#include <math.h>

namespace SDR {

void MuxFMS::init() {
  constexpr float As = 60.0f; // stop-band attenuation [dB]

  // Stereo filters / shifters
  float firStereoCutoff = 16000.0f / float(audioSampleRate_);
  // filter transition
  const float ft = 1000.0f / float(audioSampleRate_);
  // fractional timing offset
  const float mu = 0.0f;

  if (firStereoCutoff < 0) {
    firStereoCutoff = 0;
  }

  if (firStereoCutoff > 0.5) {
    firStereoCutoff = 0.5;
  }

  const unsigned int h_len = estimate_req_filter_len(ft, As);
  float* h = new float[h_len];
  liquid_firdes_kaiser(h_len, firStereoCutoff, As, mu, h);

  firStereoLeft_ = firfilt_rrrf_create(h, h_len);
  firStereoRight_ = firfilt_rrrf_create(h, h_len);

  if (demph_) {
    const double f = (1.0 / (2.0 * M_PI * double(demph_) * 1e-6));
    double t = 1.0 / (2.0 * M_PI * f);
    t = 1.0 /
        (2.0 * double(audioSampleRate_) *
         tan(1.0 / (2.0 * double(audioSampleRate_) * t)));

    const double tb = (1.0 + 2.0 * t * double(audioSampleRate_));
    float b_demph[2] = {(float)(1.0 / tb), (float)(1.0 / tb)};
    float a_demph[2] = {
        1.0, (float)((1.0 - 2.0 * t * double(audioSampleRate_)) / tb)};

    iirDemphL_ = iirfilt_rrrf_create(b_demph, 2, a_demph, 2);
    iirDemphR_ = iirfilt_rrrf_create(b_demph, 2, a_demph, 2);
  } else {
    iirDemphL_ = nullptr;
    iirDemphR_ = nullptr;
  }
}

void MuxFMS::process() {
  auto& inMono = getData<IN_MONO>();
  auto& inStereo = getData<IN_STEREO>();

  std::vector<float> outData;
  outData.reserve(inMono.size() * 2);

  for (size_t idx = 0; idx < inMono.size() && idx < inStereo.size(); ++idx) {
    float l, r;
    float ld, rd;

    if (demph_) {
      iirfilt_rrrf_execute(
          iirDemphL_, 0.568f * (inMono[idx] - (inStereo[idx])), &ld);
      iirfilt_rrrf_execute(
          iirDemphR_, 0.568f * (inMono[idx] + (inStereo[idx])), &rd);

      firfilt_rrrf_push(firStereoLeft_, ld);
      firfilt_rrrf_execute(firStereoLeft_, &l);

      firfilt_rrrf_push(firStereoRight_, rd);
      firfilt_rrrf_execute(firStereoRight_, &r);
    } else {
      firfilt_rrrf_push(
          firStereoLeft_, 0.568f * (inMono[idx] - (inStereo[idx])));
      firfilt_rrrf_execute(firStereoLeft_, &l);

      firfilt_rrrf_push(
          firStereoRight_, 0.568f * (inMono[idx] + (inStereo[idx])));
      firfilt_rrrf_execute(firStereoRight_, &r);
    }

    outData.push_back(l);
    outData.push_back(r);
  }

  setData<OUT_OUTPUT>(std::move(outData));
}

void MuxFMS::destroy() {
  firfilt_rrrf_destroy(firStereoLeft_);
  firStereoLeft_ = nullptr;
  firfilt_rrrf_destroy(firStereoRight_);
  firStereoRight_ = nullptr;
  if (iirDemphR_) {
    iirfilt_rrrf_destroy(iirDemphR_);
    iirDemphR_ = nullptr;
  }
  if (iirDemphL_) {
    iirfilt_rrrf_destroy(iirDemphL_);
    iirDemphL_ = nullptr;
  }
}

} // namespace SDR
