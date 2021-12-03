//
//  Queue.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/1/21.
//

#include "Queue.hpp"

namespace SDR {

void BaseQueue::addObserver(BaseQueueObserver* observer) {
  std::lock_guard<std::mutex> lock(observersMutex_);
  if (observers_.find(observer) != observers_.end()) {
    throw std::runtime_error("observer already added");
  }
  observers_.insert(observer);
}

void BaseQueue::removeObserver(BaseQueueObserver* observer) {
  std::lock_guard<std::mutex> lock(observersMutex_);
  if (observers_.find(observer) == observers_.end()) {
    throw std::runtime_error("observer not added");
  }
  observers_.erase(observer);
}

void BaseQueue::notifyDataAvailable() {
  std::lock_guard<std::mutex> lock(observersMutex_);
  for (const auto& observer : observers_) {
    observer->onDataAvailable(this);
  }
}

} // namespace SDR
