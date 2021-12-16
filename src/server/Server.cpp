//
//  Server.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/2/21.
//

#include "Server.hpp"

#include "ModemContext.hpp"
#include "OnDemandMetadataSubsession.hpp"
#include "OnDemandModemSubsession.hpp"

#include <boost/algorithm/string.hpp>
#include <BasicUsageEnvironment/BasicUsageEnvironment.hh>
#include <groupsock/NetAddress.hh>
#include <liveMedia/RTSPCommon.hh>
#include <liveMedia/liveMedia.hh>

namespace Tuner {

namespace {

#define MEDIA_SERVER_VERSION_STRING "0.99"

class DynamicRTSPServer : public RTSPServer {
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
      int ourSocket4,
      int ourSocket6,
      Port ourPort,
      UserAuthenticationDatabase* authDatabase,
      SDRDevice* device,
      unsigned reclamationTestSeconds);
  // called only by createNew();
  virtual ~DynamicRTSPServer();

 protected:
  virtual void lookupServerMediaSession(char const* streamName,
					lookupServerMediaSessionCompletionFunc* completionFunc,
					void* completionClientData,
					Boolean isFirstLookupInSession) override;
  virtual GenericMediaServer::ClientConnection* createNewClientConnection(
      int clientSocket,
      struct sockaddr_storage const& clientAddr) override;
  virtual GenericMediaServer::ClientSession* createNewClientSession(u_int32_t sessionId) override;

private:
  struct CompletionData {
    DynamicRTSPServer *pthis;
    void* completionClientData;
    lookupServerMediaSessionCompletionFunc* completionFunc;
    std::string streamName;
    Boolean isFirstLookupInSession;
  };
  struct CmdLookupData {
    std::string fullRequest;
  };

private:
  ServerMediaSession* setupMediaSession(
    char const* streamName,
    ServerMediaSession* sms,
    Boolean isFirstLookupInSession);
  static void myLookupCompletionFunc(void* opaque, ServerMediaSession* session);
  static void myCmdLookupCompletionFunc(void* opaque, ServerMediaSession* session);

 public:
  class CustomRTSPClientConnection : public RTSPServer::RTSPClientConnection {
    friend class DynamicRTSPServer;

   protected:
    CustomRTSPClientConnection(
        RTSPServer& ourServer,
        int clientSocket,
        struct sockaddr_storage const& clientAddr)
        : RTSPServer::RTSPClientConnection(
              ourServer,
              clientSocket,
              clientAddr) {}
    virtual void handleCmd_SET_PARAMETER(char const* fullRequestStr) override;
  };

  class CustomRTSPClientSession : public RTSPServer::RTSPClientSession {
    friend class DynamicRTSPServer;

   protected:
    CustomRTSPClientSession(RTSPServer& ourServer, u_int32_t sessionId)
        : RTSPServer::RTSPClientSession(
              ourServer,
              sessionId) {}
    virtual void handleCmd_SET_PARAMETER(
        RTSPServer::RTSPClientConnection* ourClientConnection,
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
  int ourSocket4 = setUpOurSocket(env, ourPort, AF_INET);
  int ourSocket6 = setUpOurSocket(env, ourPort, AF_INET6);
  if (ourSocket4 < 0 && ourSocket6 < 0)
    return NULL;

  return new DynamicRTSPServer(
      env, ourSocket4, ourSocket6, ourPort, authDatabase, device, reclamationTestSeconds);
}

DynamicRTSPServer::DynamicRTSPServer(
    UsageEnvironment& env,
    int ourSocket4,
    int ourSocket6,
    Port ourPort,
    UserAuthenticationDatabase* authDatabase,
    SDRDevice* device,
    unsigned reclamationTestSeconds)
    : RTSPServer(
          env,
          ourSocket4,
          ourSocket6,
          ourPort,
          authDatabase,
          reclamationTestSeconds),
      device_(device) {}

DynamicRTSPServer::~DynamicRTSPServer() {}

void DynamicRTSPServer::myLookupCompletionFunc(void* clientData,
						    ServerMediaSession* sessionLookedUp) {
  CompletionData *completionData = (CompletionData*)clientData;
  sessionLookedUp = completionData->pthis->setupMediaSession(completionData->streamName.c_str(), sessionLookedUp, completionData->isFirstLookupInSession);
  (*completionData->completionFunc)(completionData->completionClientData, sessionLookedUp);
  delete completionData;
}

void DynamicRTSPServer::lookupServerMediaSession(char const* streamName,
					lookupServerMediaSessionCompletionFunc* completionFunc,
					void* completionClientData,
					Boolean isFirstLookupInSession /*= True*/) {
    CompletionData *completionData = new CompletionData { .pthis = this, .completionClientData = completionClientData,
                  .completionFunc = completionFunc, .streamName = streamName, .isFirstLookupInSession = isFirstLookupInSession };
    RTSPServer::lookupServerMediaSession(streamName, myLookupCompletionFunc, completionData);
}

ServerMediaSession* DynamicRTSPServer::setupMediaSession(
    char const* streamName,
    ServerMediaSession* sms,
    Boolean isFirstLookupInSession) {
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
    struct sockaddr_storage const& clientAddr) {
  return new CustomRTSPClientConnection(*this, clientSocket, clientAddr);
}

GenericMediaServer::ClientSession* DynamicRTSPServer::createNewClientSession(
    u_int32_t sessionId) {
  return new CustomRTSPClientSession(*this, sessionId);
}

void DynamicRTSPServer::CustomRTSPClientConnection::handleCmd_SET_PARAMETER(
    char const* fullRequestStr) {
  RTSPServer::
      RTSPClientConnection::handleCmd_SET_PARAMETER(
          fullRequestStr);
}

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

void DynamicRTSPServer::myCmdLookupCompletionFunc(void* opaque, ServerMediaSession* session) {
  CmdLookupData *data = (CmdLookupData*)opaque;
    ServerMediaSubsessionIterator iter(*session);
    ServerMediaSubsession* subsession = nullptr;
    while ((subsession = iter.next())) {
      OnDemandModemSubsession* modemSubsession =
          dynamic_cast<OnDemandModemSubsession*>(subsession);
      if (modemSubsession) {
        const auto paramUpdates = parseRTSPHeaders(data->fullRequest.c_str());
        for (const auto& paramUpdate : paramUpdates) {
          modemSubsession->context()->queueParamUpdate(paramUpdate);
        }
        break;
      }
    }
  delete data;
}

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
  Boolean urlIsRTSPS = False;
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
          contentLength, urlIsRTSPS)) {
    CmdLookupData *data = new CmdLookupData { .fullRequest = fullRequestStr };
    fOurServer.lookupServerMediaSession(urlSuffix, myCmdLookupCompletionFunc, data, False);
   }
  RTSPServer::RTSPClientSession::handleCmd_SET_PARAMETER(
      ourClientConnection, subsession_, fullRequestStr);
}

} // namespace

void Server::setupServer() {
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  env_ = BasicUsageEnvironment::createNew(*scheduler);

  const auto rtspServer =
      DynamicRTSPServer::createNew(*env_, 554, nullptr, device_);
  if (rtspServer == NULL) {
    *env_ << "Failed to create RTSP server: " << env_->getResultMsg() << "\n";
  }
  // TODO bubble error to the app
}

void Server::runner() {
  setupServer();
  env_->taskScheduler().doEventLoop(&stopping_);
}

void Server::startRunning() {
  stopping_ = 0;
  thread_ = std::thread(&Server::runner, this);
}

void Server::stopRunning() {
  stopping_ = 1;
  thread_.join();
}

} // namespace Tuner
