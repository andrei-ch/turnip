#include "ModemAudioSource.hpp"

#include "ProgramMetadataSource.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

EventTriggerId ModemAudioSource::eventTriggerId = 0;

unsigned ModemAudioSource::referenceCount = 0;

ModemAudioSource::ModemAudioSource(
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

ModemAudioSource::~ModemAudioSource() {
  if (context_->tuner()) {
    context_->tuner()->removeObserver(this);
  }

  --referenceCount;

  if (referenceCount == 0) {
    // Reclaim our 'event trigger'
    envir().taskScheduler().deleteEventTrigger(eventTriggerId);
    eventTriggerId = 0;
  }
}

void ModemAudioSource::onAudioDataAvailable(SDR::BaseTuner* tuner) {
  assert(tuner == context_->tuner());
  envir().taskScheduler().triggerEvent(ModemAudioSource::eventTriggerId, this);
}

void ModemAudioSource::onStopped(SDR::BaseTuner* tuner) {
  assert(tuner == context_->tuner());
  tuner->removeObserver(this);
}

void ModemAudioSource::doGetNextFrame() {
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

void ModemAudioSource::deliverFrame0() {
  if (!context_->isTunerActive()) {
    return;
  }

  while (isCurrentlyAwaitingData()) {
    const auto audioData = context_->tryFetchAudioPacket();
    if (!audioData) {
      return;
    }
    deliverFrame(*audioData);
  }
}

void ModemAudioSource::deliverFrame0(void* clientData) {
  ((ModemAudioSource*)clientData)->deliverFrame0();
}

void ModemAudioSource::deliverFrame(const SDR::MP3Packet& audioData) {
  if (!isCurrentlyAwaitingData()) {
    std::cerr << "Sink is not ready, discarding frame #" << audioData.sequence
              << std::endl;
    return; // we're not ready for the data yet
  }

  if (fLastSeenSequence && !audioData.discontinuity) {
    ++fLastSeenSequence;
    if (fLastSeenSequence != audioData.sequence) {
      std::cerr << "Frame sequence number does not match. Expected: "
                << fLastSeenSequence << " Received: " << audioData.sequence
                << std::endl;
    }
  }
  fLastSeenSequence = audioData.sequence;

  fFrameSize = static_cast<unsigned int>(audioData.data.size());

  assert(fMaxSize >= fFrameSize);

  memcpy(fTo, audioData.data.data(), fFrameSize);

  // Set the 'presentation time' and 'duration' of this frame:
  if (audioData.discontinuity ||
      (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0)) {
    // This is the first frame, so use the current time:
    gettimeofday(&fPresentationTime, NULL);
  } else {
    // Increment by the play time of the previous data:
    unsigned uSeconds = fPresentationTime.tv_usec + fLastPlayTime;
    fPresentationTime.tv_sec += uSeconds / 1000000;
    fPresentationTime.tv_usec = uSeconds % 1000000;

#if DEBUG_TIMING
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);

    long presentationUSeconds =
        fPresentationTime.tv_sec * 1000000 + fPresentationTime.tv_usec;
    long currentUSeconds = currentTime.tv_sec * 1000000 + currentTime.tv_usec;

    printf(
        "presentationUSeconds is behind by %ld us\n",
        currentUSeconds - presentationUSeconds);
#endif
  }

  // Remember the play time of this data:
  fDurationInMicroseconds = fLastPlayTime =
      audioData.n_samples * 10000 / (samplingFrequency() / 100);

  // Inform the downstream object that it has data:
  FramedSource::afterGetting(this);
}
