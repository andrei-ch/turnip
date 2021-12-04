//
//  Convert.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/2/21.
//

#include "Convert.hpp"

#include <complex>

namespace SDR {

namespace {

template <typename T0, typename T1>
constexpr T1 convert(T0 sample);

template <>
inline constexpr int16_t convert<uint8_t, int16_t>(uint8_t sample) {
  return static_cast<uint16_t>(sample - 128) << 8;
}

template <>
inline constexpr float convert<uint8_t, float>(uint8_t sample) {
  return (sample - 128) / 128.f;
}

template <>
inline constexpr uint8_t convert<int16_t, uint8_t>(int16_t sample) {
  return (sample >> 8) + 128;
}

template <>
inline constexpr float convert<int16_t, float>(int16_t sample) {
  return static_cast<float>(sample) / 32768.f;
}

template <>
inline constexpr uint8_t convert<float, uint8_t>(float sample) {
  return static_cast<uint8_t>((sample * 127.f) + 128);
}

template <>
inline constexpr int16_t convert<float, int16_t>(float sample) {
  return static_cast<int16_t>(sample * 32767.f);
}

template <>
inline constexpr float convert<float, float>(float sample) {
  return sample;
}

static_assert(convert<uint8_t, int16_t>(0) == -32768);
static_assert(convert<uint8_t, int16_t>(64) == -16384);
static_assert(convert<uint8_t, int16_t>(128) == 0);
static_assert(convert<uint8_t, int16_t>(192) == 16384);
static_assert(convert<uint8_t, int16_t>(255) == 32512);
static_assert(convert<uint8_t, float>(0) == -1.f);
static_assert(convert<uint8_t, float>(64) == -.5f);
static_assert(convert<uint8_t, float>(128) == 0.f);
static_assert(convert<uint8_t, float>(192) == .5f);
static_assert(convert<uint8_t, float>(255) == .9921875f);
static_assert(convert<int16_t, uint8_t>(-32768) == 0);
static_assert(convert<int16_t, uint8_t>(-16384) == 64);
static_assert(convert<int16_t, uint8_t>(0) == 128);
static_assert(convert<int16_t, uint8_t>(16384) == 192);
static_assert(convert<int16_t, uint8_t>(32767) == 255);
static_assert(convert<int16_t, float>(-32768) == -1.f);
static_assert(convert<int16_t, float>(-16384) == -.5f);
static_assert(convert<int16_t, float>(0) == 0.f);
static_assert(convert<int16_t, float>(16384) == .5f);
static_assert(convert<int16_t, float>(32767) == .9999694824f);
static_assert(convert<float, uint8_t>(-1.f) == 1);
static_assert(convert<float, uint8_t>(-.5f) == 64);
static_assert(convert<float, uint8_t>(0.f) == 128);
static_assert(convert<float, uint8_t>(.5f) == 191);
static_assert(convert<float, uint8_t>(1.f) == 255);
static_assert(convert<float, int16_t>(-1.f) == -32767);
static_assert(convert<float, int16_t>(-.5f) == -16383);
static_assert(convert<float, int16_t>(0.f) == 0);
static_assert(convert<float, int16_t>(.5f) == 16383);
static_assert(convert<float, int16_t>(1.f) == 32767);

template <typename T1, typename T2>
void convert_vector(const std::vector<T1>& inData, std::vector<T2>& outData) {
  outData.reserve(inData.size());
  for (auto sample : inData) {
    outData.push_back(convert<T1, T2>(sample));
  }
}

template <typename T1>
void convert_vector(
    const std::vector<T1>& inData,
    std::vector<std::complex<float>>& outData) {
  outData.reserve(inData.size() >> 1);
  for (auto it = inData.begin(); it != inData.end(); it += 2) {
    outData.emplace_back(
        convert<T1, float>(*it), convert<T1, float>(*(it + 1)));
  }
}

template <typename T2>
void convert_vector(
    const std::vector<std::complex<float>>& inData,
    std::vector<T2>& outData) {
  outData.reserve(inData.size() << 1);
  for (const auto& sample : inData) {
    outData.push_back(convert<float, T2>(sample.real()));
    outData.push_back(convert<float, T2>(sample.imag()));
  }
}

} // namespace

template <typename T1, typename T2>
void Convert<T1, T2>::process() {
  auto& inData = this->template getData<IN_INPUT>();

  std::vector<T2> outData;
  convert_vector(inData, outData);

  this->template setData<OUT_OUTPUT>(std::move(outData));
}

template class Convert<uint8_t, int16_t>;
template class Convert<uint8_t, float>;
template class Convert<uint8_t, std::complex<float>>;
template class Convert<int16_t, uint8_t>;
template class Convert<int16_t, float>;
template class Convert<int16_t, std::complex<float>>;
template class Convert<float, uint8_t>;
template class Convert<float, int16_t>;
template class Convert<float, std::complex<float>>;
template class Convert<std::complex<float>, uint8_t>;
template class Convert<std::complex<float>, int16_t>;
template class Convert<std::complex<float>, float>;

} // namespace SDR
