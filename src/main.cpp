//
//  main.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 12/29/20.
//

#include "server/Server.hpp"

#include <sdrplay/api.hpp>
#include <sdrplay/device.hpp>

#include <signal.h>
#include <iostream>

namespace {

bool stopping = false;
std::mutex signal_mutex;
std::condition_variable signal_condition;

void signal_handler(int s) {
  std::lock_guard<std::mutex> lock(signal_mutex);
  stopping = true;
  signal_condition.notify_one();
}

} // namespace

// TODO implement command line bells and whistles

int main(int argc, const char* argv[]) {
  std::cout << "Searching for devices...." << std::endl;

  sdrplay::api::lock();
  const auto devices = sdrplay::api::list_devices();
  if (devices.empty()) {
    std::cout << "No device found" << std::endl;
    return 1;
  }
  std::cout << "Found device(s)" << std::endl;
  const auto device = devices.front();
  device->select();
  sdrplay::api::unlock();

  Tuner::Server server(device.get());
  server.startRunning();

  // TODO exit immediately if port 544 is already in use -> important for
  // automation

  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = signal_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);

  std::cout << "Press Ctrl+C to stop" << std::endl;

  std::unique_lock<std::mutex> lock(signal_mutex);
  signal_condition.wait(lock, []() { return stopping; });

  std::cout << "Shutting down" << std::endl;
  server.stopRunning();
  return 0;
}
