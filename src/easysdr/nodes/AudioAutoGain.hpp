//
//  AudioAutoGain.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/2/21.
//

#pragma once

#include "easysdr/core/Node.hpp"

#include <vector>

namespace SDR {

class AudioAutoGain final
    : public Node<Input<std::vector<float>>, Output<std::vector<float>>> {
 public:
  AudioAutoGain() {}

  enum { IN_INPUT = 0, OUT_OUTPUT };

  virtual void init() override;
  virtual void process() override;

 private:
  float gain_ = 1.f;
};

} // namespace SDR
