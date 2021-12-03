
#include <liveMedia/OnDemandServerMediaSubsession.hh>

#include "ModemAudioSourceParams.hpp"

#include <memory>

class ModemContext;

class OnDemandMetadataSubsession : public OnDemandServerMediaSubsession {
 public:
  OnDemandMetadataSubsession(
      UsageEnvironment& env,
      Boolean reuseFirstSource,
      const std::shared_ptr<ModemContext>& context);

 protected:
  virtual FramedSource* createNewStreamSource(
      unsigned clientSessionId,
      unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(
      Groupsock* rtpGroupsock,
      unsigned char rtpPayloadTypeIfDynamic,
      FramedSource* inputSource);

 protected:
  std::shared_ptr<ModemContext> context_;
};
