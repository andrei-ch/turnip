//
//  Liquid.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/1/21.
//

#pragma once

#if defined(__LIQUID_H__)
#error To prevent linkage errors, liquid.h must not be included directly. Include Liquid.hpp instead
#endif

// clang-format off
#include <complex>
#include <liquid/liquid.h>
// clang-format on

static_assert(
    std::is_same<liquid_float_complex, std::complex<float>>::value,
    "liquid_float_complex is not same as std::complex<float>");

static_assert(
    std::is_same<liquid_double_complex, std::complex<double>>::value,
    "liquid_double_complex is not same as std::complex<double>");

namespace liquid {

// Multi-Stage Arbitrary Resampler

template <typename T>
class msresampler final {
 public:
  msresampler(float ratio, float as = 60.0);

  size_t estimate(size_t size);
  size_t execute(size_t size, const T* in, T* out);
};

} // namespace liquid

#include "LiquidImpl.hpp"
