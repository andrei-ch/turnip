//
//  MP3Encode.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/5/21.
//

#include "MP3Encode.hpp"

#include <boost/format.hpp>
#include <iostream>

namespace SDR {

void MP3Encode::init() {
  encoder_ = lame_init();
  lame_set_in_samplerate(encoder_, sampleRate());
  lame_set_out_samplerate(encoder_, sampleRate());
  lame_set_num_channels(encoder_, numChannels());
  lame_set_quality(encoder_, 2 /* near-best quality, not too slow */);
  lame_set_brate(encoder_, bitrateKbps());
  lame_init_params(encoder_);
  sequence_ = 1;
}

void MP3Encode::process() {
  bool discontinuity = isConnected<IN_DISCONTINUITY>() &&
      hasData<IN_DISCONTINUITY>() && getData<IN_DISCONTINUITY>();

  if (!hasData<IN_INPUT>()) {
    assert(!discontinuity);
    return;
  }

  auto& inData = getData<IN_INPUT>();
  if (inData.empty()) {
    return;
  }

  int bytesEncoded;
  if (numChannels_ == 2) {
    bytesEncoded = lame_encode_buffer_interleaved(
        encoder_,
        inData.data(),
        static_cast<int>(inData.size() / 2),
        buffer_.data(),
        static_cast<int>(buffer_.size()));
  } else {
    bytesEncoded = lame_encode_buffer(
        encoder_,
        inData.data(),
        nullptr,
        static_cast<int>(inData.size()),
        buffer_.data(),
        static_cast<int>(buffer_.size()));
  }

  if (bytesEncoded < 0) {
    std::string error;
    switch (bytesEncoded) {
      case LAME_GENERICERROR:
        error = "LAME_GENERICERROR";
        break;
      case LAME_NOMEM:
        error = "LAME_NOMEM";
        break;
      case LAME_BADBITRATE:
        error = "LAME_BADBITRATE";
        break;
      case LAME_BADSAMPFREQ:
        error = "LAME_BADSAMPFREQ";
        break;
      case LAME_INTERNALERROR:
        error = "LAME_INTERNALERROR";
        break;
      case FRONTEND_READERROR:
        error = "FRONTEND_READERROR";
        break;
      case FRONTEND_WRITEERROR:
        error = "FRONTEND_WRITEERROR";
        break;
      case FRONTEND_FILETOOLARGE:
        error = "FRONTEND_FILETOOLARGE";
        break;
      default:
        error = (boost::format("%d") % bytesEncoded).str();
    }
    throw std::runtime_error("LAME encoder error " + error);
  }

  auto ptr = buffer_.begin();
  const auto ptrEnd = buffer_.begin() + bytesEncoded;

  std::vector<MP3Packet> packets;
  packets.reserve(4);

  while (ptr != ptrEnd) {
    const size_t bytesToCopy = std::min(
        static_cast<size_t>(ptrEnd - ptr),
        bytesInPacket() - packet_.data.size());

    packet_.data.insert(packet_.data.end(), ptr, ptr + bytesToCopy);
    ptr += bytesToCopy;

    if (packet_.data.size() != bytesInPacket()) {
      break;
    }

    if (packet_.data[0] != 0xff) {
      std::cerr << "Invalid packet ouput seq #" << packet_.sequence
                << std::endl;
    }

    if (packet_.data[2] & 2) {
      if (ptr == ptrEnd) {
        break;
      }
      packet_.data.push_back(*ptr++);
    }

    packet_.n_samples = samplesInPacket();
    packet_.sequence = sequence_++;
    packet_.discontinuity = discontinuity;

    packets.push_back(std::move(packet_));

    packet_.data.clear();
    discontinuity = false;
  }

  if (!packets.empty()) {
    setData<OUT_OUTPUT>(std::move(packets));
  }
}

void MP3Encode::destroy() {
  lame_close(encoder_);
  encoder_ = nullptr;
}

} // namespace SDR
