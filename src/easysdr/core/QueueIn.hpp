//
//  QueueIn.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/4/21.
//

#pragma once

#include "Node.hpp"
#include "Queue.hpp"

#include <iostream>

namespace SDR {

template <typename DataType>
class QueueIn final : public Node<Output<DataType>> {
 public:
  using QueueType = Queue<DataType>;

  QueueIn(QueueType& queue) : queue_(queue) {}

  enum { OUT_OUTPUT = 0 };

  virtual void process() override {
    auto dataPtr = queue_.try_pop();
    if (!dataPtr) {
      std::cerr << "Queue is empty: " << queue_ << std::endl << std::endl;
      return;
    }
    this->template setData<OUT_OUTPUT>(std::move(*dataPtr));
  }

 private:
  QueueType& queue_;
};

} // namespace SDR
