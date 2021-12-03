//
//  MP3Encode.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/5/21.
//

#pragma once

#include "easysdr/core/Node.hpp"

#include <lame/lame.h>

#include <array>
#include <memory>
#include <vector>

namespace SDR {

struct MP3Packet {
  size_t n_samples = 0;
  size_t sequence = 0;
  std::vector<uint8_t> data;
  bool discontinuity = false;
};

class MP3Encode final : public Node<
                            Input<std::vector<int16_t>>,
                            Input<bool>,
                            Output<std::vector<MP3Packet>>> {
 public:
  MP3Encode(
      unsigned int audioSampleRate,
      unsigned int numChannels = 1,
      unsigned int bitrateKbps = 128)
      : sampleRate_(audioSampleRate),
        numChannels_(numChannels),
        bitrateKbps_(bitrateKbps) {}

  enum { IN_INPUT = 0, IN_DISCONTINUITY, OUT_OUTPUT };

  virtual void init() override;
  virtual void process() override;
  virtual void destroy() override;

  unsigned int bitrateKbps() const {
    return bitrateKbps_;
  }

 protected:
  unsigned int sampleRate() const {
    return sampleRate_;
  }

  unsigned int numChannels() const {
    return numChannels_;
  }

  size_t bytesInPacket() const {
    // Does not include optional padding (+1 byte)
    return bitrateKbps() * 144000 / sampleRate();
  }

  constexpr static size_t samplesInPacket() {
    return 1152;
  }

 private:
  const unsigned int sampleRate_;
  const unsigned int numChannels_;
  const unsigned int bitrateKbps_;
  lame_global_flags* encoder_ = nullptr;
  std::array<uint8_t, LAME_MAXMP3BUFFER> buffer_;
  MP3Packet packet_;
  size_t sequence_;
};

} // namespace SDR
