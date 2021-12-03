//
//  DemodulateFMS.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/6/21.
//

#pragma once

#include "easysdr/core/Node.hpp"

#include "Liquid.hpp"

#include <vector>

namespace SDR {

class DemodulateFMS final
    : public Node<Input<std::vector<float>>, Output<std::vector<float>>> {
 public:
  DemodulateFMS(unsigned int bandwidth) : bandwidth_(bandwidth) {}

  enum { IN_INPUT = 0, OUT_OUTPUT };

  virtual void init() override;
  virtual void process() override;
  virtual void destroy() override;

 private:
  unsigned int bandwidth_;
  firhilbf firStereoR2C_ = nullptr;
  firhilbf firStereoC2R_ = nullptr;
  iirfilt_crcf iirStereoPilot_ = nullptr;
  nco_crcf stereoPilot_ = nullptr;
};

} // namespace SDR
