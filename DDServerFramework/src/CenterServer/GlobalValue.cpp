#include "GlobalValue.h"

WrapLog::PTR                                    gDailyLogger;
brynet::TimerMgr::PTR                           gLogicTimerMgr;
brynet::net::WrapTcpService::PTR                gServer;
brynet::net::EventLoop::PTR                     gMainLoop;