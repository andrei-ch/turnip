#pragma once

#include "tuners/HDRadioTuner.hpp"

#include <liveMedia/FramedSource.hh>
#include <memory>

class ModemContext;

class ProgramMetadataSource : public FramedSource, public SDR::TunerEvents {
 public:
  static EventTriggerId eventTriggerId;
  // Note that this is defined here to be a static class variable, because this
  // code is intended to illustrate how to encapsulate a *single* device - not a
  // set of devices. You can, however, redefine this to be a non-static member
  // variable.

  ProgramMetadataSource(
      UsageEnvironment& env,
      const std::shared_ptr<ModemContext>& context);
  // called only by createNew(), or by subclass constructors
  virtual ~ProgramMetadataSource();

  // Tuner events
  virtual void onProgramMetadataAvailable(SDR::BaseTuner* tuner);
  virtual void onStopped(SDR::BaseTuner* tuner);

 private:
  // redefined virtual functions:
  virtual void doGetNextFrame();

 private:
  void deliverFrame(
      const std::vector<std::shared_ptr<SDR::MetadataPacket>>& packets);
  void deliverFrame0();
  static void deliverFrame0(void* clientData);

 private:
  static unsigned referenceCount; // used to count how many instances of this
                                  // class currently exist
  std::shared_ptr<ModemContext> context_;
  bool addedObserver_ = false;
};
