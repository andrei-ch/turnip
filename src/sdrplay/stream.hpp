//
//  stream.hpp
//  sdrplay
//
//  Created by Andrei Chtcherbatchenko on 5/13/21.
//

#pragma once

#include "base_stream.hpp"

#include <boost/circular_buffer.hpp>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace sdrplay {

template <typename T>
struct sample_buffer {
  std::vector<T> samples;
  bool discontinuity = false;
};

template <typename T>
class stream final : public base_stream {
 public:
  stream(
      const std::shared_ptr<device>& device,
      size_t queue_size = 64,
      size_t samples_per_buffer = 32768);

  std::shared_ptr<const sample_buffer<T>> read_next_buffer();

 protected:
  virtual void reset() override;
  virtual void process_data(short* xi, short* xq, size_t num_samples) override;

 private:
  size_t samples_per_buffer() const;
  void alloc_buffer();
  void commit_buffer();
  const std::shared_ptr<sample_buffer<T>>& buffer() const;

  static void convert_samples(
      short* xi,
      short* xq,
      std::vector<T>& samples,
      size_t num_to_copy);

 private:
  std::mutex mutex_;
  std::condition_variable condition_;
  const size_t samples_per_buffer_;
  std::shared_ptr<sample_buffer<T>> buffer_;
  boost::circular_buffer<std::shared_ptr<sample_buffer<T>>> queue_;
};

template <typename T>
inline size_t stream<T>::samples_per_buffer() const {
  return samples_per_buffer_;
}

template <typename T>
inline const std::shared_ptr<sample_buffer<T>>& stream<T>::buffer() const {
  return buffer_;
}

} // namespace sdrplay
