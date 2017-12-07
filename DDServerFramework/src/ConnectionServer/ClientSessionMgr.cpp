#include <unordered_map>

#include "ClientSessionMgr.h"

static std::unordered_map<int64_t, ConnectionClientSession::PTR>   gAllClientObject;   //���еĿͻ��˶���,keyΪ����ʱID
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
        //ֻҪ����,����������Ͷ�ݷ�����ҶϿ��Ļص�
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