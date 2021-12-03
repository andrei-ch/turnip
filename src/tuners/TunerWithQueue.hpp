//
//  TunerWithQueue.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/4/21.
//

#pragma once

#include "BaseTuner.hpp"

#include <easysdr/core/Queue.hpp>

namespace SDR {

template <class AudioPacket, class MetadataPacket>
class TunerWithQueue : public BaseTuner, public BaseQueueObserver {
 public:
  TunerWithQueue()
      : audioQueue_(std::make_shared<AudioQueue>(256)),
        metadataQueue_(std::make_shared<MetadataQueue>(256)) {
    audioQueue()->addObserver(this);
    metadataQueue()->addObserver(this);
  }

  virtual ~TunerWithQueue() {
    audioQueue()->removeObserver(this);
    metadataQueue()->removeObserver(this);
  }

  virtual void onDataAvailable(const BaseQueue* queue) {
    if (audioQueue().get() == queue) {
      notifyObservers([this](TunerEvents* observer) {
        observer->onAudioDataAvailable(this);
      });
    }
    if (metadataQueue().get() == queue) {
      notifyObservers([this](TunerEvents* observer) {
        observer->onProgramMetadataAvailable(this);
      });
    }
  }

  using AudioQueue = SDR::Queue<AudioPacket>;
  using MetadataQueue = SDR::Queue<MetadataPacket>;

  const std::shared_ptr<AudioQueue>& audioQueue() const {
    return audioQueue_;
  }

  const std::shared_ptr<MetadataQueue>& metadataQueue() const {
    return metadataQueue_;
  }

  std::shared_ptr<AudioPacket> tryFetchAudioPacket() {
    return audioQueue()->try_pop();
  }

  std::shared_ptr<MetadataPacket> tryFetchMetadataPacket() {
    return metadataQueue()->try_pop();
  }

 private:
  std::shared_ptr<AudioQueue> audioQueue_;
  std::shared_ptr<MetadataQueue> metadataQueue_;
};

} // namespace SDR
