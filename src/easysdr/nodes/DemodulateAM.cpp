//
//  DemodulateAM.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/2/21.
//

#include "DemodulateAM.hpp"

namespace SDR {

void DemodulateAM::init() {
  initDemodulator(mode());
  observeControls();
}

void DemodulateAM::process() {
  auto& inData = getData<IN_INPUT>();

  std::vector<float> outData;
  outData.resize(inData.size());

  if (dc_blocker_) {
    for (size_t idx = 0; idx < inData.size(); idx++) {
      const float i = inData[idx].real();
      const float q = inData[idx].imag();
      firfilt_rrrf_push(dc_blocker_, sqrt(i * i + q * q));
      firfilt_rrrf_execute(dc_blocker_, &outData[idx]);
    }
  } else {
    for (size_t idx = 0; idx < inData.size(); idx++) {
      ampmodem_demodulate(demodulator_, inData[idx], &outData[idx]);
    }
  }

  setData<OUT_OUTPUT>(std::move(outData));
}

void DemodulateAM::destroy() {
  unobserveControls();
  destroyDemodulator();
}

void DemodulateAM::initDemodulator(unsigned int mode) {
  switch (mode) {
    case MODE_LSB:
      demodulator_ = ampmodem_create(0.5, LIQUID_AMPMODEM_LSB, 0);
      break;
    case MODE_USB:
      demodulator_ = ampmodem_create(0.5, LIQUID_AMPMODEM_USB, 0);
      break;
    case MODE_DSB:
      demodulator_ = ampmodem_create(0.5, LIQUID_AMPMODEM_DSB, 0);
      break;
    case MODE_AM:
    default:
      dc_blocker_ = firfilt_rrrf_create_dc_blocker(25, 30.f);
      break;
  }
}

void DemodulateAM::destroyDemodulator() {
  if (dc_blocker_) {
    firfilt_rrrf_destroy(dc_blocker_);
    dc_blocker_ = nullptr;
  }
  if (demodulator_) {
    ampmodem_destroy(demodulator_);
    demodulator_ = nullptr;
  }
}

void DemodulateAM::observeControls() {
  observe<CTRL_MODE>([this](unsigned int mode) {
    destroyDemodulator();
    initDemodulator(mode);
  });
}

void DemodulateAM::unobserveControls() {
  unobserve<CTRL_MODE>();
}

} // namespace SDR
