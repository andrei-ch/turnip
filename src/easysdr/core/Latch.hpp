//
//  Graph.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/1/21.
//

#include <condition_variable>
#include <mutex>

namespace SDR {

class Latch final {
 public:
  Latch() {}

  void reset(size_t count);
  void arrive_and_wait();
  void wait();

 private:
  size_t count_ = 0;
  bool done_ = false;
  std::mutex count_mutex_;
  std::mutex done_mutex_;
  std::condition_variable count_condition_;
  std::condition_variable done_condition_;
};

inline void Latch::reset(size_t count) {
  count_ = count;
  done_ = false;
}

} // namespace SDR
