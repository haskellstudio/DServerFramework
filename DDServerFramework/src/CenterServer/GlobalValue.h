#ifndef _LOGIC_SERVER_GLOBAL_
#define _LOGIC_SERVER_GLOBAL_

#include "WrapTCPService.h"
#include "WrapLog.h"
#include "Timer.h"
#include "../../ServerConfig/ServerConfig.pb.h"

extern WrapLog::PTR                                         gDailyLogger;
extern brynet::TimerMgr::PTR                                gLogicTimerMgr;
extern brynet::net::WrapTcpService::PTR                                  gServer;

/*  主线程,其他扩展模块可以引用它,譬如集成外部数据库时,可以在数据库线程向此主线程投递异步回调  */
extern brynet::net::EventLoop::PTR                                       gMainLoop;

#endif