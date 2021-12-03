//
//  BaseTuner.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/3/21.
//

#pragma once

#include "TunerParams.hpp"

#include <easysdr/core/Graph.hpp>

#include <functional>
#include <list>

namespace SDR {

class BaseTuner;

class TunerEvents {
 public:
  virtual void onStarted(BaseTuner* tuner) {}
  virtual void onAudioDataAvailable(BaseTuner* tuner) {}
  virtual void onProgramMetadataAvailable(BaseTuner* tuner) {}
  virtual void onStopped(BaseTuner* tuner) {}
};

class BaseTuner {
 public:
  BaseTuner() {}
  virtual ~BaseTuner();

  void addObserver(TunerEvents* observer);
  void removeObserver(TunerEvents* observer);

  void startRunning();
  void stopRunning();
  bool isRunning() const;

  void postControlUpdates(const TunerParams& params);

  const SDR::Graph& graph() const;
  SDR::Graph& graph();

  virtual unsigned int audioSamplingRate() const = 0;
  virtual unsigned int outputBitrateKbps() const = 0;

 protected:
  void notifyObservers(std::function<void(TunerEvents*)> lambda);

 private:
  std::list<TunerEvents*> observers_;
  bool running_ = false;
  SDR::Graph graph_;
};

inline bool BaseTuner::isRunning() const {
  return running_;
}

inline const SDR::Graph& BaseTuner::graph() const {
  return graph_;
}

inline SDR::Graph& BaseTuner::graph() {
  return graph_;
}

} // namespace SDR
