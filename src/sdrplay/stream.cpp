//
//  stream.cpp
//  sdrplay
//
//  Created by Andrei Chtcherbatchenko on 5/13/21.
//

#include "stream.hpp"

// clang-format off
#include <complex>
#include <liquid/liquid.h>
// clang-format on

static_assert(
    std::is_same<liquid_float_complex, std::complex<float>>::value,
    "liquid_float_complex must be same as std::complex<float>");

namespace sdrplay {

namespace iq {

template <typename T0, typename T1>
constexpr T1 convert(T0 sample);

template <>
inline constexpr uint8_t convert<short, uint8_t>(short sample) {
  return static_cast<uint8_t>(((sample << 2) >> 8) + 128);
}

template <>
inline constexpr float convert<short, float>(short sample) {
  return static_cast<float>(sample) / 32768.0f;
}

template <typename T>
inline constexpr
    typename std::enable_if<std::is_arithmetic<T>::value, size_t>::type
    elems_per_sample() {
  return 2;
}

template <typename T>
inline constexpr typename std::
    enable_if<std::is_same<T, std::complex<float>>::value, size_t>::type
    elems_per_sample() {
  return 1;
}

} // namespace iq

template <typename T>
stream<T>::stream(
    const std::shared_ptr<device>& device,
    size_t queue_size /*= 64*/,
    size_t samples_per_buffer /*= 32768*/)
    : base_stream(device),
      queue_(queue_size),
      samples_per_buffer_(samples_per_buffer) {
  reset();
}

template <typename T>
std::shared_ptr<const sample_buffer<T>> stream<T>::read_next_buffer() {
  std::unique_lock<std::mutex> lock(mutex_);
  condition_.wait(lock, [this]() { return !queue_.empty(); });
  const auto ptr = queue_.front();
  queue_.pop_front();
  return ptr;
}

template <typename T>
void stream<T>::alloc_buffer() {
  assert(!buffer_);
  buffer_ = std::make_shared<sample_buffer<T>>();
  buffer()->samples.reserve(samples_per_buffer() * iq::elems_per_sample<T>());
}

template <typename T>
void stream<T>::commit_buffer() {
  bool notify;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.full()) {
      queue_.pop_front();
      queue_.front()->discontinuity = true;
    }
    notify = queue_.empty();
    queue_.push_back(buffer());
    buffer_ = nullptr;
  }
  if (notify) {
    condition_.notify_one();
  }
}

template <typename T>
void stream<T>::reset() {
  if (buffer()) {
    buffer()->samples.clear();
  } else {
    alloc_buffer();
  }
  buffer()->discontinuity = true;
}

template <typename T>
void stream<T>::process_data(short* xi, short* xq, size_t num_samples) {
  constexpr size_t elems_per_sample = iq::elems_per_sample<T>();
  const size_t max_samples = samples_per_buffer();

  assert(buffer() && (buffer()->samples.size() % elems_per_sample) == 0);

  while (num_samples > 0) {
    const size_t num_to_copy = std::min(
        num_samples, max_samples - buffer()->samples.size() / elems_per_sample);

    convert_samples(xi, xq, buffer()->samples, num_to_copy);

    num_samples -= num_to_copy;
    xi += num_to_copy;
    xq += num_to_copy;

    if (buffer()->samples.size() == max_samples * elems_per_sample) {
      commit_buffer();
      alloc_buffer();
    }
  }
}

template <>
void stream<uint8_t>::convert_samples(
    short* xi,
    short* xq,
    std::vector<uint8_t>& samples,
    size_t num_to_copy) {
  for (size_t i = 0; i < num_to_copy; ++i) {
    samples.push_back(iq::convert<short, uint8_t>(xi[i]));
    samples.push_back(iq::convert<short, uint8_t>(xq[i]));
  }
}

template <>
void stream<int16_t>::convert_samples(
    short* xi,
    short* xq,
    std::vector<int16_t>& samples,
    size_t num_to_copy) {
  for (size_t i = 0; i < num_to_copy; ++i) {
    samples.push_back(xi[i]);
    samples.push_back(xq[i]);
  }
}

template <>
void stream<float>::convert_samples(
    short* xi,
    short* xq,
    std::vector<float>& samples,
    size_t num_to_copy) {
  for (size_t i = 0; i < num_to_copy; ++i) {
    samples.push_back(iq::convert<short, float>(xi[i]));
    samples.push_back(iq::convert<short, float>(xq[i]));
  }
}

template <>
void stream<std::complex<float>>::convert_samples(
    short* xi,
    short* xq,
    std::vector<std::complex<float>>& samples,
    size_t num_to_copy) {
  for (size_t i = 0; i < num_to_copy; ++i) {
    samples.emplace_back(
        iq::convert<short, float>(xi[i]), iq::convert<short, float>(xq[i]));
  }
}

template class stream<uint8_t>;
template class stream<int16_t>;
template class stream<float>;
template class stream<std::complex<float>>;

} // namespace sdrplay
