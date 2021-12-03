//
//  RTSPServer.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/2/21.
//

#pragma once

#include <atomic>
#include <thread>

class UsageEnvironment;

namespace sdrplay {
class device;
}

namespace Tuner {

using SDRDevice = sdrplay::device;

class RTSPServer {
 public:
  RTSPServer(SDRDevice* device) : device_(device) {}

  void startRunning();
  void stopRunning();

 private:
  void runner();
  void setupServer();

 private:
  SDRDevice* device_;
  std::thread thread_;
  UsageEnvironment* env_ = nullptr;
  volatile char stopping_ = 0;
};

} // namespace Tuner
