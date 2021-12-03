#include "OnDemandModemSubsession.hpp"

#include "ModemAudioSource.hpp"

#include <liveMedia/MP3ADU.hh>
#include <liveMedia/MP3ADURTPSink.hh>
#include <liveMedia/MPEG1or2AudioRTPSink.hh>

OnDemandModemSubsession::OnDemandModemSubsession(
    UsageEnvironment& env,
    Boolean reuseFirstSource,
    const std::shared_ptr<ModemContext>& context)
    : OnDemandServerMediaSubsession(env, reuseFirstSource),
      context_(context),
      weakContext_(context) {}

FramedSource* OnDemandModemSubsession ::createNewStreamSource(
    unsigned /*clientSessionId*/,
    unsigned& estBitrate) {
  if (!context_) {
    return NULL;
  }

  FramedSource* resultSource = NULL;
  do {
    ModemAudioSource* wavSource = new ModemAudioSource(envir(), context_);
    if (wavSource == NULL)
      break;

      // Add in any filter necessary to transform the data prior to streaming:
#if USE_ADU
    resultSource = ADUFromMP3Source::createNew(envir(), wavSource);
#else
    resultSource = wavSource; // by default
#endif

    estBitrate = context_->outputBitrateKbps();

    context_.reset();
    return resultSource;
  } while (0);

  // An error occurred:
  Medium::close(resultSource);
  return NULL;
}

RTPSink* OnDemandModemSubsession ::createNewRTPSink(
    Groupsock* rtpGroupsock,
    unsigned char rtpPayloadTypeIfDynamic,
    FramedSource* /*inputSource*/) {
  do {
    char const* mimeType;
#if USE_ADU
    mimeType = "MPA-ROBUST";
    return MP3ADURTPSink::createNew(
        envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
#else
    mimeType = "MPA";
    return MPEG1or2AudioRTPSink::createNew(envir(), rtpGroupsock);
#endif
  } while (0);

  // An error occurred:
  return NULL;
}
