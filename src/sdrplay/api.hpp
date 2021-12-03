//
//  api.hpp
//  sdrplay
//
//  Created by Andrei Chtcherbatchenko on 5/13/21.
//

#pragma once

#include <memory>
#include <vector>

namespace sdrplay {

class device;

class api final {
 private:
  api() = delete;

 public:
  static void lock();
  static void unlock();
  static std::vector<std::shared_ptr<device>> list_devices();

 private:
  friend class device;
  static void select(device* dev);
  static void release(device* dev);
};

} // namespace sdrplay
