#include <unordered_map>

#include "LogicServerSessionMgr.h"

std::unordered_map<int, LogicServerSession::PTR>        gAllPrimaryServers;
std::mutex                                              gAllPrimaryServersLock;

std::unordered_map<int, LogicServerSession::PTR>        gAllSlaveServers;
std::mutex                                              gAllSlaveServersLock;

void LogicServerSessionMgr::AddPrimaryLogicServer(int id, LogicServerSession::PTR p)
{
    gAllPrimaryServersLock.lock();
    gAllPrimaryServers[id] = p;
    gAllPrimaryServersLock.unlock();
}

LogicServerSession::PTR LogicServerSessionMgr::FindPrimaryLogicServer(int id)
{
    LogicServerSession::PTR tmp;

    gAllPrimaryServersLock.lock();
    tmp = gAllPrimaryServers[id];
    gAllPrimaryServersLock.unlock();

    return tmp;
}

void LogicServerSessionMgr::RemovePrimaryLogicServer(int id)
{
    gAllPrimaryServersLock.lock();
    gAllPrimaryServers.erase(id);
    gAllPrimaryServersLock.unlock();
}

int LogicServerSessionMgr::ClaimPrimaryLogicServer()
{
    int ret = -1;
    gAllPrimaryServersLock.lock();
    if (!gAllPrimaryServers.empty())
    {
        int randnum = rand() % gAllPrimaryServers.size();
        int i = 0;
        BaseNetSession::PTR    logicServer = nullptr;
        for (auto& v : gAllPrimaryServers)
        {
            if (i++ == randnum)
            {
                ret = v.first;
                break;
            }
        }
    }
    gAllPrimaryServersLock.unlock();

    return ret;
}

void LogicServerSessionMgr::AddSlaveLogicServer(int id, LogicServerSession::PTR p)
{
    gAllSlaveServersLock.lock();
    gAllSlaveServers[id] = p;
    gAllSlaveServersLock.unlock();
}

LogicServerSession::PTR LogicServerSessionMgr::FindSlaveLogicServer(int id)
{
    LogicServerSession::PTR tmp;

    gAllSlaveServersLock.lock();
    tmp = gAllSlaveServers[id];
    gAllSlaveServersLock.unlock();

    return tmp;
}

void LogicServerSessionMgr::RemoveSlaveLogicServer(int id)
{
    gAllSlaveServersLock.lock();
    gAllSlaveServers.erase(id);
    gAllSlaveServersLock.unlock();
}
