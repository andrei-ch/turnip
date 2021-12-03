
#include <liveMedia/OnDemandServerMediaSubsession.hh>

#include <memory>

class ModemContext;

class OnDemandModemSubsession : public OnDemandServerMediaSubsession {
 public:
  OnDemandModemSubsession(
      UsageEnvironment& env,
      Boolean reuseFirstSource,
      const std::shared_ptr<ModemContext>& context);

  std::shared_ptr<ModemContext> context() const {
    return weakContext_.lock();
  }

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
  std::weak_ptr<ModemContext> weakContext_;
};
