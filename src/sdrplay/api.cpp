//
//  api.cpp
//  sdrplay
//
//  Created by Andrei Chtcherbatchenko on 5/13/21.
//

#include "api.hpp"

#include "device.hpp"

#include <array>
#include <unordered_set>

namespace sdrplay {

namespace {

class api_impl {
 public:
  api_impl() {
    if (sdrplay_api_Open() != sdrplay_api_Success) {
      throw std::runtime_error("sdrplay_api_Open");
    }
  }

  ~api_impl() {
    for (auto dev : selected_devices_) {
      sdrplay_api_ReleaseDevice(dev->ptr());
    }
    if (num_locks_ > 0) {
      sdrplay_api_UnlockDeviceApi();
    }
    sdrplay_api_Close();
  }

  void lock() {
    if (num_locks_++ == 0) {
      sdrplay_api_LockDeviceApi();
    }
  }

  void unlock() {
    assert(num_locks_ > 0);
    if (--num_locks_ == 0) {
      sdrplay_api_UnlockDeviceApi();
    }
  }

  void select(device* dev) {
    assert(selected_devices_.find(dev) == selected_devices_.end());
    if (sdrplay_api_SelectDevice(dev->ptr()) != sdrplay_api_Success) {
      throw std::runtime_error("sdrplay_api_SelectDevice");
    }
    selected_devices_.insert(dev);
  }

  void release(device* dev) {
    assert(selected_devices_.find(dev) != selected_devices_.end());
    if (sdrplay_api_ReleaseDevice(dev->ptr()) != sdrplay_api_Success) {
      throw std::runtime_error("sdrplay_api_ReleaseDevice");
    }
    selected_devices_.erase(dev);
  }

  static api_impl* singleton() {
    static std::unique_ptr<api_impl> singleton_;
    if (!singleton_) {
      singleton_ = std::make_unique<api_impl>();
    }
    return singleton_.get();
  }

  static void init() {
    (void)singleton();
  }

 private:
  unsigned int num_locks_ = 0;
  std::unordered_set<device*> selected_devices_;
};

struct device_desc_list {
  std::array<sdrplay_api_DeviceT, 16> buffer;
  unsigned int count = 0;
};

} // namespace

void api::lock() {
  api_impl::singleton()->lock();
}

void api::unlock() {
  api_impl::singleton()->unlock();
}

void api::select(device* dev) {
  api_impl::singleton()->select(dev);
}

void api::release(device* dev) {
  api_impl::singleton()->release(dev);
}

std::vector<std::shared_ptr<device>> api::list_devices() {
  api_impl::init();
  auto devices = std::make_shared<device_desc_list>();
  if (sdrplay_api_GetDevices(
          devices->buffer.data(),
          &devices->count,
          static_cast<unsigned int>(devices->buffer.size())) !=
      sdrplay_api_Success) {
    throw std::runtime_error("sdrplay_api_GetDevices");
  }
  std::vector<std::shared_ptr<device>> out;
  for (size_t idx = 0; idx < devices->count; ++idx) {
    const auto device_ptr = std::shared_ptr<sdrplay_api_DeviceT>(
        &devices->buffer[idx], [devices](auto) {});
    out.push_back(std::make_shared<device>(device_ptr));
  }
  return out;
}

} // namespace sdrplay
