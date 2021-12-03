//
//  BaseTuner.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/3/21.
//

#include "BaseTuner.hpp"

namespace SDR {

BaseTuner::~BaseTuner() {
  stopRunning();
  graph().unbindAll();
  assert(observers_.empty());
}

void BaseTuner::startRunning() {
  if (running_) {
    return;
  }
  graph_.startRunning();
  running_ = true;
  notifyObservers([this](TunerEvents* observer) { observer->onStarted(this); });
  std::cout << "Tuner started" << std::endl;
}

void BaseTuner::stopRunning() {
  if (!running_) {
    return;
  }
  graph_.stopRunning();
  running_ = false;
  notifyObservers([this](TunerEvents* observer) { observer->onStopped(this); });
  std::cout << "Tuner stopped" << std::endl;
}

void BaseTuner::postControlUpdates(const TunerParams& params) {
  auto updates = params.toAnyMap();
  graph().postUpdates(std::move(updates));
}

void BaseTuner::addObserver(TunerEvents* observer) {
  const auto found = std::find(observers_.begin(), observers_.end(), observer);
  if (found != observers_.end()) {
    std::cout << "Warning: observer already added" << std::endl;
    return;
  }
  observers_.push_back(observer);
}

void BaseTuner::removeObserver(TunerEvents* observer) {
  const auto found = std::find(observers_.begin(), observers_.end(), observer);
  if (found == observers_.end()) {
    std::cout << "Warning: observer must be added first" << std::endl;
    return;
  }
  observers_.erase(found);
}

void BaseTuner::notifyObservers(std::function<void(TunerEvents*)> lambda) {
  auto cur = observers_.begin();
  while (cur != observers_.end()) {
    const auto next = std::next(cur);
    lambda(*cur);
    cur = next;
  }
}

} // namespace SDR
