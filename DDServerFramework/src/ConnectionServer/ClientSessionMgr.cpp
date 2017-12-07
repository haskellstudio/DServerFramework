#include <unordered_map>

#include "ClientSessionMgr.h"

static std::unordered_map<int64_t, ConnectionClientSession::PTR>   gAllClientObject;   //所有的客户端对象,key为运行时ID
static std::mutex                                                  gAllClientObjectLock;

ConnectionClientSession::PTR ClientSessionMgr::FindClientByRuntimeID(int64_t runtimeID)
{
    ConnectionClientSession::PTR ret = nullptr;

    std::lock_guard<std::mutex> lck(gAllClientObjectLock);

    auto it = gAllClientObject.find(runtimeID);
    if (it != gAllClientObject.end())
    {
        ret = it->second;
    }

    return ret;
}

void ClientSessionMgr::AddClientByRuntimeID(ConnectionClientSession::PTR client, int64_t runtimeID)
{
    assert(FindClientByRuntimeID(runtimeID) == nullptr);
    std::lock_guard<std::mutex> lck(gAllClientObjectLock);
    gAllClientObject[runtimeID] = client;
}

void ClientSessionMgr::EraseClientByRuntimeID(int64_t runtimeID)
{
    std::lock_guard<std::mutex> lck(gAllClientObjectLock);
    gAllClientObject.erase(runtimeID);
}

void ClientSessionMgr::KickClientByRuntimeID(int64_t runtimeID)
{
    ConnectionClientSession::PTR p = FindClientByRuntimeID(runtimeID);
    if (p != nullptr)
    {
        p->postDisConnect();
        //只要被踢,无条件立即投递发送玩家断开的回调
        p->getEventLoop()->pushAsyncProc([=]() {
            p->notifyExit();
        });
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