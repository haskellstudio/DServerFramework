#ifndef _LOGICSERVER_SESSION_MGR_H
#define _LOGICSERVER_SESSION_MGR_H

#include "LogicServerSession.h"

class LogicServerSessionMgr
{
public:
    static  void    AddPrimaryLogicServer(int id, LogicServerSession::PTR);
    static  LogicServerSession::PTR FindPrimaryLogicServer(int id);

    static  void    RemovePrimaryLogicServer(int id);
    static  int     ClaimPrimaryLogicServer();

    static  void    AddSlaveLogicServer(int id, LogicServerSession::PTR);
    static  LogicServerSession::PTR FindSlaveLogicServer(int id);
    static  void    RemoveSlaveLogicServer(int id);
};

#endif