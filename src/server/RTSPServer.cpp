//
//  RTSPServer.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/2/21.
//

#include "RTSPServer.hpp"

#include "ModemContext.hpp"
#include "OnDemandMetadataSubsession.hpp"
#include "OnDemandModemSubsession.hpp"

#include <boost/algorithm/string.hpp>
#include <BasicUsageEnvironment/BasicUsageEnvironment.hh>
#include <liveMedia/RTSPCommon.hh>
#include <liveMedia/liveMedia.hh>

namespace Tuner {

namespace {

#define MEDIA_SERVER_VERSION_STRING "0.99"

class DynamicRTSPServer : public RTSPServerSupportingHTTPStreaming {
 public:
  static DynamicRTSPServer* createNew(
      UsageEnvironment& env,
      Port ourPort,
      UserAuthenticationDatabase* authDatabase,
      SDRDevice* device,
      unsigned reclamationTestSeconds = 65);

 protected:
  DynamicRTSPServer(
      UsageEnvironment& env,
      int ourSocket,
      Port ourPort,
      UserAuthenticationDatabase* authDatabase,
      SDRDevice* device,
      unsigned reclamationTestSeconds);
  // called only by createNew();
  virtual ~DynamicRTSPServer();

 protected: // redefined virtual functions
  virtual ServerMediaSession* lookupServerMediaSession(
      char const* streamName,
      Boolean isFirstLookupInSession);
  virtual ClientConnection* createNewClientConnection(
      int clientSocket,
      struct sockaddr_in clientAddr);
  virtual ClientSession* createNewClientSession(u_int32_t sessionId);

  class CustomRTSPClientConnection
      : public RTSPClientConnectionSupportingHTTPStreaming {
    friend class DynamicRTSPServer;

   protected:
    CustomRTSPClientConnection(
        RTSPServer& ourServer,
        int clientSocket,
        struct sockaddr_in clientAddr)
        : RTSPClientConnectionSupportingHTTPStreaming(
              ourServer,
              clientSocket,
              clientAddr) {}
    virtual void handleCmd_SET_PARAMETER(char const* fullRequestStr);
  };

  class CustomRTSPClientSession : public RTSPClientSession {
    friend class DynamicRTSPServer;

   protected:
    CustomRTSPClientSession(RTSPServer& ourServer, u_int32_t sessionId)
        : RTSPServerSupportingHTTPStreaming::RTSPClientSession(
              ourServer,
              sessionId) {}
    virtual void handleCmd_SET_PARAMETER(
        RTSPClientConnection* ourClientConnection,
        ServerMediaSubsession* subsession,
        char const* fullRequestStr);
  };

 private:
  SDRDevice* device_;
};

DynamicRTSPServer* DynamicRTSPServer::createNew(
    UsageEnvironment& env,
    Port ourPort,
    UserAuthenticationDatabase* authDatabase,
    SDRDevice* device,
    unsigned reclamationTestSeconds) {
  int ourSocket = setUpOurSocket(env, ourPort);
  if (ourSocket == -1)
    return NULL;

  return new DynamicRTSPServer(
      env, ourSocket, ourPort, authDatabase, device, reclamationTestSeconds);
}

DynamicRTSPServer::DynamicRTSPServer(
    UsageEnvironment& env,
    int ourSocket,
    Port ourPort,
    UserAuthenticationDatabase* authDatabase,
    SDRDevice* device,
    unsigned reclamationTestSeconds)
    : RTSPServerSupportingHTTPStreaming(
          env,
          ourSocket,
          ourPort,
          authDatabase,
          reclamationTestSeconds),
      device_(device) {}

DynamicRTSPServer::~DynamicRTSPServer() {}

ServerMediaSession* DynamicRTSPServer::lookupServerMediaSession(
    char const* streamName,
    Boolean isFirstLookupInSession) {
  ServerMediaSession* sms = RTSPServer::lookupServerMediaSession(streamName);
  std::shared_ptr<ModemContext> modemContext;

  try {
    ModemAudioSourceParams params(device_, streamName);
    modemContext = std::make_shared<ModemContext>(params);
  } catch (const std::exception& ex) {
    std::cerr << "Error parsing URI " << streamName << " : " << ex.what()
              << std::endl;
  }

  if (!modemContext) {
    if (sms) {
      removeServerMediaSession(sms);
    }
    return nullptr;
  }

  if (sms && isFirstLookupInSession) {
    removeServerMediaSession(sms);
    sms = nullptr;
  }

  if (!sms) {
    sms = ServerMediaSession::createNew(
        envir(), streamName, streamName, "SDR Audio Stream");

    modemContext->ensureNoActiveTuners();

    sms->addSubsession(
        new OnDemandModemSubsession(envir(), False, modemContext));

    sms->addSubsession(
        new OnDemandMetadataSubsession(envir(), False, modemContext));

    addServerMediaSession(sms);
  }
  return sms;
}

GenericMediaServer::ClientConnection*
DynamicRTSPServer::createNewClientConnection(
    int clientSocket,
    struct sockaddr_in clientAddr) {
  return new CustomRTSPClientConnection(*this, clientSocket, clientAddr);
}

GenericMediaServer::ClientSession* DynamicRTSPServer::createNewClientSession(
    u_int32_t sessionId) {
  return new CustomRTSPClientSession(*this, sessionId);
}

void DynamicRTSPServer::CustomRTSPClientConnection::handleCmd_SET_PARAMETER(
    char const* fullRequestStr) {
  RTSPServerSupportingHTTPStreaming::
      RTSPClientConnectionSupportingHTTPStreaming::handleCmd_SET_PARAMETER(
          fullRequestStr);
}

namespace {
static std::vector<std::string> parseRTSPHeaders(char const* fullRequestStr) {
  std::vector<std::string> out;
  std::istringstream resp(fullRequestStr);
  std::string header;
  std::string::size_type index;
  while (std::getline(resp, header) && header != "\r") {
    index = header.find(':', 0);
    if (index != std::string::npos) {
      auto key = boost::algorithm::trim_copy(header.substr(0, index));
      if (key == "x-turnip-set-parameters") {
        const auto value =
            boost::algorithm::trim_copy(header.substr(index + 1));
        out.push_back(value);
      }
    }
  }
  return out;
}
} // namespace

void DynamicRTSPServer::CustomRTSPClientSession::handleCmd_SET_PARAMETER(
    RTSPClientConnection* ourClientConnection,
    ServerMediaSubsession* subsession_,
    char const* fullRequestStr) {
  char cmdName[RTSP_PARAM_STRING_MAX];
  char urlPreSuffix[RTSP_PARAM_STRING_MAX];
  char urlSuffix[RTSP_PARAM_STRING_MAX];
  char cseq[RTSP_PARAM_STRING_MAX];
  char sessionId[RTSP_PARAM_STRING_MAX];
  unsigned contentLength;
  if (parseRTSPRequestString(
          fullRequestStr,
          strlen(fullRequestStr),
          cmdName,
          sizeof cmdName,
          urlPreSuffix,
          sizeof urlPreSuffix,
          urlSuffix,
          sizeof urlSuffix,
          cseq,
          sizeof cseq,
          sessionId,
          sizeof sessionId,
          contentLength)) {
    ServerMediaSession* session =
        fOurServer.lookupServerMediaSession(urlSuffix, False);
    ServerMediaSubsessionIterator iter(*session);
    ServerMediaSubsession* subsession = nullptr;
    while ((subsession = iter.next())) {
      OnDemandModemSubsession* modemSubsession =
          dynamic_cast<OnDemandModemSubsession*>(subsession);
      if (modemSubsession) {
        const auto paramUpdates = parseRTSPHeaders(fullRequestStr);
        for (const auto& paramUpdate : paramUpdates) {
          modemSubsession->context()->queueParamUpdate(paramUpdate);
        }
        break;
      }
    }
  }

  RTSPServerSupportingHTTPStreaming::RTSPClientSession::handleCmd_SET_PARAMETER(
      ourClientConnection, subsession_, fullRequestStr);
}

} // namespace

void RTSPServer::setupServer() {
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  env_ = BasicUsageEnvironment::createNew(*scheduler);

  const auto rtspServer =
      DynamicRTSPServer::createNew(*env_, 554, nullptr, device_);
  if (rtspServer == NULL) {
    *env_ << "Failed to create RTSP server: " << env_->getResultMsg() << "\n";
  }
  // TODO bubble error to the app
}

void RTSPServer::runner() {
  setupServer();
  env_->taskScheduler().doEventLoop(&stopping_);
}

void RTSPServer::startRunning() {
  stopping_ = 0;
  thread_ = std::thread(&RTSPServer::runner, this);
}

void RTSPServer::stopRunning() {
  stopping_ = 1;
  thread_.join();
}

} // namespace Tuner
