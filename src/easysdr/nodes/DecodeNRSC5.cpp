//
//  DecodeNRSC5.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/21/21.
//

#include "DecodeNRSC5.hpp"

#include <boost/algorithm/string.hpp>
#include <iostream>

namespace SDR {

template <typename T>
void DecodeNRSC5<T>::callback(const nrsc5_event_t* evt) {
  switch (evt->event) {
    case NRSC5_EVENT_AUDIO:
      if (evt->audio.program == program()) {
        audioBuffer_.insert(
            audioBuffer_.end(),
            evt->audio.data,
            evt->audio.data + evt->audio.count);
      }
      break;

    case NRSC5_EVENT_SYNC:
      std::cerr << "NRSC5 sync" << std::endl;
      audioBuffer_.clear();
      discontinuity_ = true;
      break;

    case NRSC5_EVENT_LOST_SYNC:
      std::cerr << "NRSC5 lost sync" << std::endl;
      break;

    case NRSC5_EVENT_BER: {
      const float ber = evt->ber.cber;
      ber_sum_ += ber;
      ++ber_count_;
      ber_min_ = std::min(ber_min_, ber);
      ber_max_ = std::max(ber_max_, ber);
      setMetadata("nrsc5.ber", ber);
      setMetadata("nrsc5.ber_avg", ber_sum_ / ber_count_);
      setMetadata("nrsc5.ber_min", ber_min_);
      setMetadata("nrsc5.ber_max", ber_max_);
      break;
    }

    case NRSC5_EVENT_MER:
      setMetadata("nrsc5.mer_lower", evt->mer.lower);
      setMetadata("nrsc5.mer_upper", evt->mer.upper);
      break;

    case NRSC5_EVENT_ID3:
      if (evt->id3.program == program()) {
        setMetadata("id3.title", evt->id3.title);
        setMetadata("id3.artist", evt->id3.artist);
        setMetadata("id3.album", evt->id3.album);
        setMetadata("id3.genre", evt->id3.genre);
        setMetadata("id3.ufid_owner", evt->id3.ufid.owner);
        setMetadata("id3.ufid_id", evt->id3.ufid.id);
        // TODO process XHDR frames
      }
      break;

    case NRSC5_EVENT_SIG: {
      // TODO process SIG packets
      break;
    }

    case NRSC5_EVENT_LOT: {
      // TODO process LOT packets
      break;
    }

    case NRSC5_EVENT_SIS: {
      // TODO process SIG packets
      break;
    }
  }
}

template <typename T>
void DecodeNRSC5<T>::setMetadata(const char* key, const char* value) {
  if (!value) {
    return;
  }
  std::string s = value;
  boost::algorithm::trim(s);
  if (!s.empty()) {
    metadata_[key] = s;
  }
}

template <typename T>
template <typename T0>
void DecodeNRSC5<T>::setMetadata(const char* key, T0 value) {
  metadata_[key] = value;
}

template <typename T>
void DecodeNRSC5<T>::staticCallback(const nrsc5_event_t* evt, void* opaque) {
  reinterpret_cast<DecodeNRSC5*>(opaque)->callback(evt);
}

template <typename T>
void DecodeNRSC5<T>::init() {
  nrsc5_open_pipe(&decoder_);
  nrsc5_set_callback(decoder_, staticCallback, this);
  observeControls();
}

template <>
void DecodeNRSC5<uint8_t>::pipeIQData() {
  const auto& inData = this->template getData<IN_INPUT>();
  const size_t numToCopy =
      ((tempBuffer_.size() + inData.size()) & ~3) - tempBuffer_.size();
  tempBuffer_.insert(
      tempBuffer_.end(), inData.begin(), inData.begin() + numToCopy);
  nrsc5_pipe_samples_cu8(
      decoder_,
      tempBuffer_.data(),
      static_cast<unsigned int>(tempBuffer_.size()));
  tempBuffer_.assign(inData.begin() + numToCopy, inData.end());
}

template <>
void DecodeNRSC5<int16_t>::pipeIQData() {
  auto& inData = this->template getData<IN_INPUT>();
  nrsc5_pipe_samples_cs16(
      decoder_, inData.data(), static_cast<unsigned int>(inData.size()));
}

template <typename T>
void DecodeNRSC5<T>::process() {
  pipeIQData();

  if (!audioBuffer_.empty()) {
    this->template setData<OUT_OUTPUT>(std::move(audioBuffer_));
    this->template setData<OUT_DISCONTINUITY>(std::move(discontinuity_));
    discontinuity_ = false;
  }

  if (!metadata_.empty()) {
    this->template setData<OUT_METADATA>(std::move(metadata_));
    metadata_.clear();
  }
}

template <typename T>
void DecodeNRSC5<T>::destroy() {
  unobserveControls();
  nrsc5_close(decoder_);
  decoder_ = nullptr;
}

template <typename T>
void DecodeNRSC5<T>::observeControls() {
  this->template observe<CTRL_PROGRAM>([this](unsigned int program) {
    audioBuffer_.clear();
    discontinuity_ = true;
  });
}

template <typename T>
void DecodeNRSC5<T>::unobserveControls() {
  this->template unobserve<CTRL_PROGRAM>();
}

template class DecodeNRSC5<uint8_t>;
template class DecodeNRSC5<int16_t>;

} // namespace SDR
