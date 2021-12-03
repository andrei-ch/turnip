#pragma once

#include <liveMedia/FramedSource.hh>

#include "ModemAudioSourceParams.hpp"
#include "ModemContext.hpp"

class ModemAudioSource : public FramedSource, public SDR::TunerEvents {
 public:
  unsigned samplingFrequency() const {
    return context_->audioSamplingFrequency();
  }

  // Tuner events
  virtual void onAudioDataAvailable(SDR::BaseTuner* tuner);
  virtual void onStopped(SDR::BaseTuner* tuner);

 public:
  static EventTriggerId eventTriggerId;
  // Note that this is defined here to be a static class variable, because this
  // code is intended to illustrate how to encapsulate a *single* device - not a
  // set of devices. You can, however, redefine this to be a non-static member
  // variable.

  ModemAudioSource(
      UsageEnvironment& env,
      const std::shared_ptr<ModemContext>& context);
  // called only by createNew(), or by subclass constructors
  virtual ~ModemAudioSource();

 private:
  // redefined virtual functions:
  virtual void doGetNextFrame();

 private:
  void deliverFrame(const SDR::MP3Packet& data);
  void deliverFrame0();
  static void deliverFrame0(void* clientData);

 private:
  std::shared_ptr<ModemContext> context_;
  bool addedObserver_ = false;

  static unsigned referenceCount; // used to count how many instances of this
                                  // class currently exist

  unsigned fLastPlayTime;
  size_t fLastSeenSequence = 0;
};
