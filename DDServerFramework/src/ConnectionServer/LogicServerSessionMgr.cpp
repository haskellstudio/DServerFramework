#include <unordered_map>
#include "LogicServerSessionMgr.h"

using namespace brynet::net;

std::unordered_map<int, LogicServerSession::PTR>        gAllPrimaryServers;
std::mutex                                              gAllPrimaryServersLock;

std::unordered_map<int, LogicServerSession::PTR>        gAllSlaveServers;
std::mutex                                              gAllSlaveServersLock;

void LogicServerSessionMgr::AddPrimaryLogicServer(int id, LogicServerSession::PTR p)
{
    std::lock_guard<std::mutex> lck(gAllPrimaryServersLock);
    gAllPrimaryServers[id] = p;
}

LogicServerSession::PTR LogicServerSessionMgr::FindPrimaryLogicServer(int id)
{
    LogicServerSession::PTR tmp;

    std::lock_guard<std::mutex> lck(gAllPrimaryServersLock);
    tmp = gAllPrimaryServers[id];

    return tmp;
}

std::unordered_map<int, LogicServerSession::PTR> LogicServerSessionMgr::GetAllPrimaryLogicServer()
{
    std::unordered_map<int, LogicServerSession::PTR> tmp;

    gAllPrimaryServersLock.lock();
    tmp = gAllPrimaryServers;
    gAllPrimaryServersLock.unlock();

    return tmp;
}

void LogicServerSessionMgr::RemovePrimaryLogicServer(int id)
{
    std::lock_guard<std::mutex> lck(gAllPrimaryServersLock);
    gAllPrimaryServers.erase(id);
}

int LogicServerSessionMgr::ClaimPrimaryLogicServer()
{
    int ret = -1;
    std::lock_guard<std::mutex> lck(gAllPrimaryServersLock);
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

    return ret;
}

void LogicServerSessionMgr::AddSlaveLogicServer(int id, LogicServerSession::PTR p)
{
    std::lock_guard<std::mutex> lck(gAllSlaveServersLock);
    gAllSlaveServers[id] = p;
}

LogicServerSession::PTR LogicServerSessionMgr::FindSlaveLogicServer(int id)
{
    LogicServerSession::PTR tmp;

    std::lock_guard<std::mutex> lck(gAllSlaveServersLock);
    tmp = gAllSlaveServers[id];

    return tmp;
}

void LogicServerSessionMgr::RemoveSlaveLogicServer(int id)
{
    std::lock_guard<std::mutex> lck(gAllSlaveServersLock);
    gAllSlaveServers.erase(id);
}

std::unordered_map<int, LogicServerSession::PTR> LogicServerSessionMgr::GetAllSlaveLogicServer()
{
    std::unordered_map<int, LogicServerSession::PTR> tmp;

    gAllSlaveServersLock.lock();
    tmp = gAllSlaveServers;
    gAllSlaveServersLock.unlock();

    return tmp;
}