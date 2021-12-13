#include "ProgramMetadataSource.hpp"

#include "ModemContext.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

EventTriggerId ProgramMetadataSource::eventTriggerId = 0;

unsigned ProgramMetadataSource::referenceCount = 0;

ProgramMetadataSource::ProgramMetadataSource(
    UsageEnvironment& env,
    const std::shared_ptr<ModemContext>& context)
    : FramedSource(env), context_(context) {
  if (referenceCount == 0) {
    if (eventTriggerId == 0) {
      eventTriggerId =
          envir().taskScheduler().createEventTrigger(deliverFrame0);
    }
  }

  ++referenceCount;
}

ProgramMetadataSource::~ProgramMetadataSource() {
  if (context_->tuner()) {
    context_->tuner()->removeObserver(this);
  }

  --referenceCount;

  if (referenceCount == 0) {
    envir().taskScheduler().deleteEventTrigger(eventTriggerId);
    eventTriggerId = 0;
  }
}

void ProgramMetadataSource::onProgramMetadataAvailable(SDR::BaseTuner* tuner) {
  assert(tuner == context_->tuner());
  envir().taskScheduler().triggerEvent(
      ProgramMetadataSource::eventTriggerId, this);
}

void ProgramMetadataSource::onStopped(SDR::BaseTuner* tuner) {
  assert(tuner == context_->tuner());
  tuner->removeObserver(this);
}

void ProgramMetadataSource::doGetNextFrame() {
  if (!context_->ensureTunerStarted()) {
    handleClosure();
    return;
  }

  assert(context_->tuner());

  if (!addedObserver_) {
    addedObserver_ = true;
    context_->tuner()->addObserver(this);
  }

  if (context_->isTunerStopped()) {
    handleClosure();
    return;
  }

  deliverFrame0();
}

void ProgramMetadataSource::deliverFrame0() {
  if (!context_->isTunerActive()) {
    return;
  }

  while (isCurrentlyAwaitingData()) {
    constexpr size_t maxPacketsToCoalesce = 16;

    std::vector<std::shared_ptr<SDR::MetadataPacket>> packets;
    while (packets.size() < maxPacketsToCoalesce) {
      const auto metadata = context_->tryFetchMetadataPacket();
      if (!metadata) {
        break;
      }
      packets.push_back(metadata);
    }

    if (packets.empty()) {
      return;
    }

    deliverFrame(packets);
  }
}

void ProgramMetadataSource::deliverFrame0(void* clientData) {
  ((ProgramMetadataSource*)clientData)->deliverFrame0();
}

void ProgramMetadataSource::deliverFrame(
    const std::vector<std::shared_ptr<SDR::MetadataPacket>>& packets) {
  if (!isCurrentlyAwaitingData()) {
    std::cerr << "Sink is not ready, discarding data" << std::endl;
    return; // we're not ready for the data yet
  }

  if (packets.empty()) {
    return;
  }

  SDR::MetadataPacket metadata(std::move(*packets.back()));
  for (auto it = packets.rbegin() + 1; it != packets.rend(); ++it) {
    metadata.merge(**it);
  }

  const std::string json = SDR::metadataToJsonString(metadata);

  fFrameSize = static_cast<unsigned int>(json.length());
  memcpy(fTo, json.c_str(), fFrameSize);

  gettimeofday(&fPresentationTime, NULL);

  // Inform the downstream object that it has data:
  FramedSource::afterGetting(this);
}
