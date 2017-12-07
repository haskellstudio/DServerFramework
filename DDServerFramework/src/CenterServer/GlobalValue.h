#ifndef _LOGIC_SERVER_GLOBAL_
#define _LOGIC_SERVER_GLOBAL_

#include <brynet/net/WrapTcpService.h>
#include <brynet/timer/Timer.h>
#include "WrapLog.h"
#include "../../ServerConfig/ServerConfig.pb.h"

extern WrapLog::PTR                                         gDailyLogger;
extern brynet::TimerMgr::PTR                                gLogicTimerMgr;
extern brynet::net::WrapTcpService::PTR                                  gServer;

/*  ���߳�,������չģ�����������,Ʃ�缯���ⲿ���ݿ�ʱ,���������ݿ��߳�������߳�Ͷ���첽�ص�  */
extern brynet::net::EventLoop::PTR                                       gMainLoop;

#endif