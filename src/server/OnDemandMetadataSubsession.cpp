#include "OnDemandMetadataSubsession.hpp"

#include "ProgramMetadataSource.hpp"

#include <liveMedia/SimpleRTPSink.hh>

OnDemandMetadataSubsession::OnDemandMetadataSubsession(
    UsageEnvironment& env,
    Boolean reuseFirstSource,
    const std::shared_ptr<ModemContext>& context)
    : OnDemandServerMediaSubsession(env, reuseFirstSource), context_(context) {}

FramedSource* OnDemandMetadataSubsession ::createNewStreamSource(
    unsigned /*clientSessionId*/,
    unsigned& estBitrate) {
  if (!context_) {
    return NULL;
  }
  ProgramMetadataSource* metadataSource =
      new ProgramMetadataSource(envir(), context_);
  context_.reset();
  return metadataSource;
}

RTPSink* OnDemandMetadataSubsession ::createNewRTPSink(
    Groupsock* rtpGroupsock,
    unsigned char rtpPayloadTypeIfDynamic,
    FramedSource* /*inputSource*/) {
  SimpleRTPSink* newSink = SimpleRTPSink::createNew(
      envir(),
      rtpGroupsock,
      rtpPayloadTypeIfDynamic,
      48000,
      "application",
      "SDR-META",
      1,
      False);
  return newSink;
}
