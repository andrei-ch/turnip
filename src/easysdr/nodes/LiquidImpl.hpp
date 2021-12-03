//
//  LiquidImpl.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/1/21.
//

#pragma once

#include <cmath>

namespace liquid {

// Multi-Stage Arbitrary Resampler

template <typename T, typename TR>
class msresampler_impl {
 public:
  using creator_t = TR (*)(float, float);
  using deletor_t = void (*)(TR);
  using executor_t = void (*)(TR, T*, unsigned int, T*, unsigned int*);

  msresampler_impl(
      float ratio,
      float as,
      creator_t creator,
      deletor_t deletor,
      executor_t executor)
      : ratio_(ratio),
        resamp_(creator(ratio, as)),
        deletor_(deletor),
        executor_(executor) {}

  ~msresampler_impl() {
    deletor_(resamp_);
  }

  size_t estimate(size_t size) {
    return static_cast<size_t>(std::ceil((size + 1) * ratio_)) + 1;
  }

  size_t execute(size_t size, const T* in, T* out) {
    unsigned int w;
    executor_(
        resamp_, const_cast<T*>(in), static_cast<unsigned int>(size), out, &w);
    return static_cast<size_t>(w);
  }

 private:
  float ratio_;
  TR resamp_;
  deletor_t deletor_;
  executor_t executor_;
};

template <>
class msresampler<float> : public msresampler_impl<float, msresamp_rrrf> {
 public:
  msresampler<float>(float ratio, float as)
      : msresampler_impl<float, msresamp_rrrf>(
            ratio,
            as,
            msresamp_rrrf_create,
            msresamp_rrrf_destroy,
            msresamp_rrrf_execute) {}
};

template <>
class msresampler<std::complex<float>>
    : public msresampler_impl<std::complex<float>, msresamp_crcf> {
 public:
  msresampler<std::complex<float>>(float ratio, float as)
      : msresampler_impl<std::complex<float>, msresamp_crcf>(
            ratio,
            as,
            msresamp_crcf_create,
            msresamp_crcf_destroy,
            msresamp_crcf_execute) {}
};

} // namespace liquid
