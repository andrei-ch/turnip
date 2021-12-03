//
//  Queue.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/1/21.
//

#pragma once

#include <boost/circular_buffer.hpp>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <ostream>
#include <unordered_set>
#include <vector>

namespace SDR {

class BaseQueue;

class BaseQueueObserver {
 public:
  virtual void onDataAvailable(const BaseQueue* queue) = 0;
};

class BaseQueue {
 public:
  BaseQueue() {}
  virtual ~BaseQueue() {}

  void addObserver(BaseQueueObserver* observer);
  void removeObserver(BaseQueueObserver* observer);

  virtual bool waitForData(const std::chrono::milliseconds& timeout) = 0;

 protected:
  void notifyDataAvailable();

 private:
  std::unordered_set<BaseQueueObserver*> observers_;
  std::mutex observersMutex_;
};

template <typename T>
class Queue final : public BaseQueue {
 public:
  Queue(size_t size) : data_(size) {}

  virtual bool waitForData(const std::chrono::milliseconds& timeout) override;

  void push(T&& data);
  void push(const std::shared_ptr<T>& data);
  bool try_push(T&& data);
  bool try_push(const std::shared_ptr<T>& data);
  std::shared_ptr<T> try_pop();

  friend std::ostream& operator<<(std::ostream& os, Queue& queue) {
    std::lock_guard<std::mutex> lock(queue.mutex_);
    os << "Queue type:" << typeid(T).name() << " size: " << queue.data_.size()
       << " capacity: " << queue.data_.capacity();
    return os;
  }

 private:
  boost::circular_buffer<std::shared_ptr<T>> data_;
  std::mutex mutex_;
  std::condition_variable condition_;
};

template <typename T>
bool Queue<T>::waitForData(const std::chrono::milliseconds& timeout) {
  std::unique_lock<std::mutex> lock(mutex_);
  return condition_.wait_for(lock, timeout, [this] { return !data_.empty(); });
}

template <typename T>
void Queue<T>::push(T&& data) {
  push(std::make_shared<T>(std::move(data)));
}

template <typename T>
bool Queue<T>::try_push(T&& data) {
  return try_push(std::make_shared<T>(std::move(data)));
}

template <typename T>
void Queue<T>::push(const std::shared_ptr<T>& data) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    bool was_empty = data_.empty();
    data_.push_back(data);
    if (!was_empty) {
      return;
    }
  }
  notifyDataAvailable();
}

template <typename T>
bool Queue<T>::try_push(const std::shared_ptr<T>& data) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (data_.full()) {
      return false;
    }
    bool was_empty = data_.empty();
    data_.push_back(data);
    if (!was_empty) {
      return true;
    }
  }
  notifyDataAvailable();
  return true;
}

template <typename T>
std::shared_ptr<T> Queue<T>::try_pop() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (data_.empty()) {
    return nullptr;
  }
  std::shared_ptr<T> temp = data_.front();
  data_.pop_front();
  return temp;
}

} // namespace SDR
