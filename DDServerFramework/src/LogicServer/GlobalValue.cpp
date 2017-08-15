#include "GlobalValue.h"

ServerConfig::CenterServerConfig centerServerConfig;
ServerConfig::LogicServerConfig logicServerConfig;

WrapLog::PTR                                    gDailyLogger;
brynet::TimerMgr::PTR                           gLogicTimerMgr;
WrapTcpService::PTR                             gServer;
EventLoop::PTR gMainLoop;
ClientMirrorMgr::PTR  gClientMirrorMgr;
CenterServerConnection::PTR gLogicCenterServerClient = nullptr;
dodo::rpc::RpcService<dodo::rpc::MsgpackProtocol>    gCenterServerConnectioinRpc;