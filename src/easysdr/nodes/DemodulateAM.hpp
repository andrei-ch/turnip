//
//  DemodulateAM.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/2/21.
//

#pragma once

#include "easysdr/core/Node.hpp"

#include "Liquid.hpp"

#include <vector>

namespace SDR {

class DemodulateAM final : public Node<
                               Input<std::vector<std::complex<float>>>,
                               Output<std::vector<float>>,
                               Control<unsigned int>> {
 public:
  DemodulateAM(unsigned int mode) {
    setMode(mode);
  }

  enum { IN_INPUT = 0, OUT_OUTPUT, CTRL_MODE };

  virtual void init() override;
  virtual void process() override;
  virtual void destroy() override;

  enum { MODE_AM = 0, MODE_LSB, MODE_USB, MODE_DSB };

  unsigned int mode() const;
  void setMode(unsigned int mode);

 private:
  void initDemodulator(unsigned int mode);
  void destroyDemodulator();
  void observeControls();
  void unobserveControls();

 private:
  firfilt_rrrf dc_blocker_ = nullptr;
  ampmodem demodulator_ = nullptr;
};

inline unsigned int DemodulateAM::mode() const {
  return portAt<CTRL_MODE>().value();
}

inline void DemodulateAM::setMode(unsigned int mode) {
  portAt<CTRL_MODE>().setValue(mode);
}

} // namespace SDR
