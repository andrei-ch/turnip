//
//  base_stream.cpp
//  sdrplay
//
//  Created by Andrei Chtcherbatchenko on 5/13/21.
//

#include "stream.hpp"

#include "device.hpp"

namespace sdrplay {

base_stream::~base_stream() {
  const auto device = device_.lock();
  if (device) {
    device->close_stream(this);
  }
}

} // namespace sdrplay
