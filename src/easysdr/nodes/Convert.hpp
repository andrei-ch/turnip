//
//  Convert.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/2/21.
//

#pragma once

#include "easysdr/core/Node.hpp"

#include <vector>

namespace SDR {

template <typename T1, typename T2>
class Convert final
    : public Node<Input<std::vector<T1>>, Output<std::vector<T2>>> {
 public:
  Convert() {}

  enum { IN_INPUT = 0, OUT_OUTPUT };

  virtual void process() override;
};

} // namespace SDR
