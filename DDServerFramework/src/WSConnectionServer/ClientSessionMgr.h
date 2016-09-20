#ifndef _CLIENT_SESSION_MGR_H
#define _CLIENT_SESSION_MGR_H

#include "ClientSession.h"

class ClientSessionMgr
{
public:
    static  ConnectionClientSession::PTR FindClientByRuntimeID(int64_t runtimeID);

    static  void AddClientByRuntimeID(ConnectionClientSession::PTR client, int64_t runtimeID);
    static  void EraseClientByRuntimeID(int64_t runtimeID);

    static  void KickClientByRuntimeID(int64_t runtimeID);
    static  void KickClientOfPrimary(int primaryServerID);
};

#endif