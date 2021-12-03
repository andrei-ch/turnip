//
//  Graph.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/1/21.
//

#include "Latch.hpp"

namespace SDR {

void Latch::arrive_and_wait() {
  {
    std::lock_guard<std::mutex> lock(count_mutex_);
    --count_;
  }
  count_condition_.notify_one();
  {
    std::unique_lock<std::mutex> lock(done_mutex_);
    done_condition_.wait(lock, [this] { return done_; });
  }
}

void Latch::wait() {
  {
    std::unique_lock<std::mutex> lock(count_mutex_);
    count_condition_.wait(lock, [this] { return count_ == 0; });
  }
  {
    std::lock_guard<std::mutex> lock(done_mutex_);
    done_ = true;
  }
  done_condition_.notify_all();
}

} // namespace SDR
