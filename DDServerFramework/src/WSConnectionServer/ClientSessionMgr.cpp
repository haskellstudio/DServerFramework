#include <unordered_map>

#include "ClientSessionMgr.h"

std::unordered_map<int64_t, ConnectionClientSession::PTR>   gAllClientObject;   //所有的客户端对象,key为运行时ID
std::mutex                                                  gAllClientObjectLock;
extern WrapServer::PTR                                      gServer;

ConnectionClientSession::PTR ClientSessionMgr::FindClientByRuntimeID(int64_t runtimeID)
{
    ConnectionClientSession::PTR ret = nullptr;

    gAllClientObjectLock.lock();

    auto it = gAllClientObject.find(runtimeID);
    if (it != gAllClientObject.end())
    {
        ret = it->second;
    }
    gAllClientObjectLock.unlock();

    return ret;
}

void ClientSessionMgr::AddClientByRuntimeID(ConnectionClientSession::PTR client, int64_t runtimeID)
{
    assert(FindClientByRuntimeID(runtimeID) == nullptr);
    gAllClientObjectLock.lock();
    gAllClientObject[runtimeID] = client;
    gAllClientObjectLock.unlock();
}

void ClientSessionMgr::EraseClientByRuntimeID(int64_t runtimeID)
{
    gAllClientObjectLock.lock();
    gAllClientObject.erase(runtimeID);
    gAllClientObjectLock.unlock();
}

void ClientSessionMgr::KickClientByRuntimeID(int64_t runtimeID)
{
    ConnectionClientSession::PTR p = FindClientByRuntimeID(runtimeID);
    if (p != nullptr)
    {
        p->setKicked();
        auto socketID = p->getSocketID();
        if (socketID != -1)
        {
            gServer->getService()->disConnect(socketID);
        }
    }
}

void ClientSessionMgr::KickClientOfPrimary(int primaryServerID)
{
    gAllClientObjectLock.lock();
    auto copyList = gAllClientObject;
    gAllClientObjectLock.unlock();

    for (auto& v : copyList)
    {
        if (v.second->getPrimaryServerID() == primaryServerID)
        {
            KickClientByRuntimeID(v.second->getRuntimeID());
        }
    }
}