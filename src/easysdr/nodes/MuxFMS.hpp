//
//  MuxFMS.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/6/21.
//

#pragma once

#include "easysdr/core/Node.hpp"

#include "Liquid.hpp"

#include <vector>

namespace SDR {

class MuxFMS final : public Node<
                         Input<std::vector<float>>,
                         Input<std::vector<float>>,
                         Output<std::vector<float>>> {
 public:
  MuxFMS(unsigned int audioSampleRate, int demph = 75)
      : audioSampleRate_(audioSampleRate), demph_(demph) {}

  enum { IN_MONO = 0, IN_STEREO, OUT_OUTPUT };

  virtual void init() override;
  virtual void process() override;
  virtual void destroy() override;

 private:
  unsigned int audioSampleRate_;
  int demph_;
  firfilt_rrrf firStereoLeft_ = nullptr;
  firfilt_rrrf firStereoRight_ = nullptr;
  iirfilt_rrrf iirDemphR_ = nullptr;
  iirfilt_rrrf iirDemphL_ = nullptr;
};

} // namespace SDR
