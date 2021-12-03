//
//  QueueOut.hpp
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
class QueueOut final
    : public Node<Input<DataType>, Input<std::vector<DataType>>> {
 public:
  using QueueType = Queue<DataType>;

  QueueOut(QueueType& queue) : queue_(queue) {}

  enum { IN_INPUT = 0, IN_INPUT_VECTOR };

  virtual void process() override {
    if (this->template isConnected<IN_INPUT>() &&
        this->template hasData<IN_INPUT>()) {
      auto& inData = this->template getData<IN_INPUT>();
      if (!queue_.try_push(std::move(inData))) {
        std::cerr << "Queue is full, discarding data: " << queue_ << std::endl;
      }
    }
    if (this->template isConnected<IN_INPUT_VECTOR>() &&
        this->template hasData<IN_INPUT_VECTOR>()) {
      auto& inDataVec = this->template getData<IN_INPUT_VECTOR>();
      for (auto& inData : inDataVec) {
        if (!queue_.try_push(std::move(inData))) {
          std::cerr << "Queue is full, discarding data: " << queue_
                    << std::endl;
          break;
        }
      }
    }
  }

 private:
  QueueType& queue_;
};

} // namespace SDR
