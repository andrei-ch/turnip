//
//  DemodulateFM.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/6/21.
//

#pragma once

#include "easysdr/core/Node.hpp"

#include "Liquid.hpp"

#include <vector>

namespace SDR {

class DemodulateFM final : public Node<
                               Input<std::vector<std::complex<float>>>,
                               Output<std::vector<float>>> {
 public:
  DemodulateFM() {}

  enum { IN_INPUT = 0, OUT_OUTPUT };

  virtual void init() override;
  virtual void process() override;
  virtual void destroy() override;

 private:
  freqdem demodulator_ = nullptr;
};

} // namespace SDR
