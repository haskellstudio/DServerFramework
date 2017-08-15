#ifndef _LOGIC_SERVER_GLOBAL_
#define _LOGIC_SERVER_GLOBAL_

#include "WrapTCPService.h"
#include "WrapLog.h"
#include "Timer.h"
#include "CenterServerConnection.h"
#include "ClientMirror.h"
#include "../../ServerConfig/ServerConfig.pb.h"

extern ServerConfig::LogicServerConfig                      logicServerConfig;
extern ServerConfig::CenterServerConfig                     centerServerConfig;
extern WrapLog::PTR                                         gDailyLogger;
extern brynet::TimerMgr::PTR                                gLogicTimerMgr;
extern brynet::net::WrapTcpService::PTR                     gServer;
extern brynet::net::EventLoop::PTR                          gMainLoop;

extern CenterServerConnection::PTR                          gLogicCenterServerClient;
extern dodo::rpc::RpcService<dodo::rpc::MsgpackProtocol>    gCenterServerConnectioinRpc;
extern ClientMirrorMgr::PTR                                 gClientMirrorMgr;

#endif