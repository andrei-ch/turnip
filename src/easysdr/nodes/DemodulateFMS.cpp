//
//  DemodulateFMS.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/6/21.
//

#include "DemodulateFMS.hpp"

#include <math.h>

namespace SDR {

void DemodulateFMS::init() {
  constexpr float As = 60.0f; // stop-band attenuation [dB]

  // stereo pilot filter
  float bw = float(bandwidth_);
  if (bw < 100000.0) {
    bw = 100000.0;
  }
  const unsigned int order = 5; // filter order
  const float f0 = ((float)19000 / bw);
  const float fc = ((float)19500 / bw);
  const float Ap = 1.0f;

  iirStereoPilot_ = iirfilt_crcf_create_prototype(
      LIQUID_IIRDES_CHEBY2,
      LIQUID_IIRDES_BANDPASS,
      LIQUID_IIRDES_SOS,
      order,
      fc,
      f0,
      Ap,
      As);

  firStereoR2C_ = firhilbf_create(5, 60.0f);
  firStereoC2R_ = firhilbf_create(5, 60.0f);

  stereoPilot_ = nco_crcf_create(LIQUID_VCO);
  nco_crcf_reset(stereoPilot_);
  nco_crcf_pll_set_bandwidth(stereoPilot_, 0.25f);
}

void DemodulateFMS::process() {
  auto& inData = getData<IN_INPUT>();

  std::vector<float> outData;
  outData.reserve(inData.size());

  float phase_error = 0;
  for (float inSample : inData) {
    std::complex<float> u, v, w, x, y;

    // real -> complex
    firhilbf_r2c_execute(firStereoR2C_, inSample, &x);

    // 19khz pilot band-pass
    iirfilt_crcf_execute(iirStereoPilot_, x, &v);
    nco_crcf_cexpf(stereoPilot_, &w);

    w.imag(-w.imag()); // conjf(w)

    // multiply u = v * conjf(w)
    u.real(v.real() * w.real() - v.imag() * w.imag());
    u.imag(v.real() * w.imag() + v.imag() * w.real());

    // cargf(u)
    phase_error = atan2f(u.imag(), u.real());

    // step pll
    nco_crcf_pll_step(stereoPilot_, phase_error);
    nco_crcf_step(stereoPilot_);

    // 38khz down-mix
    nco_crcf_mix_down(stereoPilot_, x, &y);
    nco_crcf_mix_down(stereoPilot_, y, &x);

    // complex -> real
    float usb_discard;
    float outSample;
    firhilbf_c2r_execute(firStereoC2R_, x, &outSample, &usb_discard);

    outData.push_back(outSample);
  }

  setData<OUT_OUTPUT>(std::move(outData));
}

void DemodulateFMS::destroy() {
  firhilbf_destroy(firStereoR2C_);
  firStereoR2C_ = nullptr;
  firhilbf_destroy(firStereoC2R_);
  firStereoC2R_ = nullptr;
  nco_crcf_destroy(stereoPilot_);
  stereoPilot_ = nullptr;
  iirfilt_crcf_destroy(iirStereoPilot_);
  iirStereoPilot_ = nullptr;
}

} // namespace SDR
