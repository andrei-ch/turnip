//
//  DecodeNRSC5.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/21/21.
//

#pragma once

#include "easysdr/core/Metadata.hpp"
#include "easysdr/core/Node.hpp"

#include <string>
#include <vector>

extern "C" {
#include <nrsc5.h>
}

namespace SDR {

template <typename T>
class DecodeNRSC5 final : public Node<
                              Input<std::vector<T>>,
                              Output<std::vector<int16_t>>,
                              Output<bool>,
                              Output<MetadataPacket>,
                              Control<unsigned int>> {
 public:
  DecodeNRSC5(unsigned int program) {
    setProgram(program);
  }

  enum {
    IN_INPUT = 0,
    OUT_OUTPUT,
    OUT_DISCONTINUITY,
    OUT_METADATA,
    CTRL_PROGRAM
  };

  virtual void init() override;
  virtual void process() override;
  virtual void destroy() override;

  unsigned int program() const;
  void setProgram(unsigned int program);

 private:
  void callback(const nrsc5_event_t* evt);
  static void staticCallback(const nrsc5_event_t* evt, void* opaque);
  void pipeIQData();
  void observeControls();
  void unobserveControls();
  void setMetadata(const char* key, const char* value);

  template <typename T0>
  void setMetadata(const char* key, T0 value);

 private:
  nrsc5_t* decoder_ = nullptr;
  std::vector<T> tempBuffer_;
  std::vector<int16_t> audioBuffer_;
  bool discontinuity_ = false;
  MetadataPacket metadata_;
  float ber_min_ = 1.f;
  float ber_max_ = 0.f;
  float ber_sum_ = 0.f;
  unsigned int ber_count_ = 0;
};

template <typename T>
inline unsigned int DecodeNRSC5<T>::program() const {
  return this->template portAt<CTRL_PROGRAM>().value();
}

template <typename T>
inline void DecodeNRSC5<T>::setProgram(unsigned int program) {
  this->template portAt<CTRL_PROGRAM>().setValue(program);
}

} // namespace SDR
