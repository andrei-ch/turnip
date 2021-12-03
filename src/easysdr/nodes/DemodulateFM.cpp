//
//  DemodulateFM.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/6/21.
//

#include "DemodulateFM.hpp"

namespace SDR {

void DemodulateFM::init() {
  demodulator_ = freqdem_create(0.5);
}

void DemodulateFM::process() {
  auto& inData = getData<IN_INPUT>();

  std::vector<float> outData;
  outData.resize(inData.size());

  freqdem_demodulate_block(
      demodulator_,
      inData.data(),
      static_cast<int>(inData.size()),
      outData.data());

  setData<OUT_OUTPUT>(std::move(outData));
}

void DemodulateFM::destroy() {
  freqdem_destroy(demodulator_);
  demodulator_ = nullptr;
}

} // namespace SDR
