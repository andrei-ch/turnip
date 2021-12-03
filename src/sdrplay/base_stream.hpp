//
//  base_stream.hpp
//  sdrplay
//
//  Created by Andrei Chtcherbatchenko on 5/13/21.
//

#pragma once

#include <memory>

namespace sdrplay {

class device;

class base_stream {
 public:
  base_stream(const std::shared_ptr<device>& device) : device_(device) {}
  virtual ~base_stream();

  virtual void reset() = 0;
  virtual void process_data(short* xi, short* xq, size_t num_samples) = 0;

 private:
  std::weak_ptr<device> device_;
};

} // namespace sdrplay
