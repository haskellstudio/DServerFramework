#include <set>
#include <stdint.h>

using namespace std;

#include "packet.h"
#include "WrapLog.h"
#include "ConnectionServerSendOP.h"

#include "ClientLogicObject.h"

extern unordered_map<int, BaseNetSession::PTR>      gAllSlaveServers;
extern unordered_map<int, BaseNetSession::PTR>      gAllPrimaryServers;
extern TimerMgr::PTR                                gTimerMgr;
extern WrapLog::PTR gDailyLogger;
extern WrapServer::PTR                              gServer;

unordered_map<int64_t, ClientObject::PTR>   gAllClientObject;   //所有的客户端对象,key为运行时ID
std::mutex                                  gAllClientObjectLock;

ClientObject::PTR findClientByRuntimeID(int64_t runtimeID)
{
    ClientObject::PTR ret = nullptr;

    gAllClientObjectLock.lock();

    auto it = gAllClientObject.find(runtimeID);
    if (it != gAllClientObject.end())
    {
        ret = it->second;
    }
    gAllClientObjectLock.unlock();

    return ret;
}

void addClientByRuntimeID(ClientObject::PTR client, int64_t runtimeID)
{
    assert(findClientByRuntimeID(runtimeID) == nullptr);
    gAllClientObjectLock.lock();
    gAllClientObject[runtimeID] = client;
    gAllClientObjectLock.unlock();

    gDailyLogger->warn("add client, runtime id :{}, current online client num:{}", runtimeID, gAllClientObject.size());
}

void eraseClientByRuntimeID(int64_t runtimeID)
{
    gAllClientObjectLock.lock();
    gAllClientObject.erase(runtimeID);
    gAllClientObjectLock.unlock();

    gDailyLogger->warn("remove client, runtime id:{}, current online client num:{}", runtimeID, gAllClientObject.size());
}

void kickClientByRuntimeID(int64_t runtimeID)
{
    ClientObject::PTR p = findClientByRuntimeID(runtimeID);
    if (p != nullptr)
    {
        int64_t socketID = p->getSocketID();
        if (socketID != -1)
        {
            gDailyLogger->warn("kick client, runtime id: {}, socket id{}", runtimeID, socketID);
            gServer->getService()->disConnect(socketID);
        }

        eraseClientByRuntimeID(runtimeID);
    }
}

void kickClientOfPrimary(int primaryServerID)
{
    gAllClientObjectLock.lock();
    auto copyList = gAllClientObject;
    gAllClientObjectLock.unlock();

    for (auto& v : copyList)
    {
        if (v.second->getPrimaryServerID() == primaryServerID)
        {
            kickClientByRuntimeID(v.second->getRuntimeID());
        }
    }
}

ClientObject::ClientObject(int64_t id)
{
    mSocketID = id;
    mRuntimeID = -1;
    mPrimaryServerID = -1;
    mSlaveServerID = -1;
}

void ClientObject::resetSocketID(int64_t id)
{
    /*重设连接id，通常是重连后重新设置，所以加下面两个断言*/
    assert(mSocketID == -1);
    mSocketID = id;
}

int64_t ClientObject::getSocketID() const
{
    return mSocketID;
}

/*当此客户端对象销毁时，通知内部服务器强制退出此RuntimeID标示的客户端--真正退出游戏*/
ClientObject::~ClientObject()
{
    gDailyLogger->info("~ client object : {}", mRuntimeID);

    if (mRuntimeID != -1)
    {
        eraseClientByRuntimeID(mRuntimeID);

        TinyPacket sp(CONNECTION_SERVER_SEND_LOGICSERVER_DESTROY_CLIENT);
        sp.writeINT64(mRuntimeID);
        sp.getLen();

        sendPacketToPrimaryServer(sp);
        sendPacketToSlaveServer(sp);

        mRuntimeID = -1;
    }
}

void ClientObject::notifyDisConnect()
{
    TinyPacket sp(CONNECTION_SERVER_SEND_LOGICSERVER_CLIENT_DISCONNECT);
    sp.writeINT64(mRuntimeID);

    sendPacketToPrimaryServer(sp);
}

int64_t ClientObject::getRuntimeID() const
{
    return mRuntimeID;
}

void ClientObject::setClosed()
{
    mSocketID = -1;
}

void ClientObject::procPacket(PACKET_OP_TYPE op, const char* packerBuffer, PACKET_OP_TYPE packetLen)
{
    if (mRuntimeID == -1)
    {
        claimRuntimeID();
    }

    if (mPrimaryServerID == -1)
    {
        claimPrimaryServer();
    }

    if (mPrimaryServerID != -1)
    {
        BigPacket packet(CONNECTION_SERVER_SEND_LOGICSERVER_FROMCLIENT);
        packet.writeINT64(mRuntimeID);
        packet.writeBinary(packerBuffer, packetLen);

        if (mSlaveServerID == -1)
        {
            sendPacketToPrimaryServer(packet);
        }
        else
        {
            sendPacketToSlaveServer(packet);
        }
    }
}

void ClientObject::setSlaveServerID(int id)
{
    gDailyLogger->info("set client {} slave server id:{}", mRuntimeID, id);
    mSlaveServerID = id;
}

int ClientObject::getPrimaryServerID() const
{
    return mPrimaryServerID;
}

/*TODO::是否保持所在的服务器会话指针，避免每次查找*/
void ClientObject::sendPacketToPrimaryServer(Packet& packet)
{
    _sendPacketToServer(packet, gAllPrimaryServers, mPrimaryServerID);
}

void ClientObject::sendPacketToSlaveServer(Packet& packet)
{
    _sendPacketToServer(packet, gAllSlaveServers, mSlaveServerID);
}

void ClientObject::_sendPacketToServer(Packet& packet, unordered_map<int32_t, BaseNetSession::PTR>& servers, int32_t serverID)
{
    BaseNetSession::PTR    logicServer = nullptr;
    auto it = servers.find(serverID);
    if (it != servers.end())
    {
        logicServer = (*it).second;
    }
    if (logicServer != nullptr)
    {
        logicServer->sendPacket(packet.getData(), packet.getLen());
    }
    else
    {
        gDailyLogger->error("server not found, id:{}, server num:{}", serverID, servers.size());
    }
}

union CLIENT_RUNTIME_ID
{
    int64_t id;
    struct
    {
        int16_t serverID;
        int16_t incID;
        int32_t time;
    }humman;
};

extern int  gSelfID;

void ClientObject::claimRuntimeID()
{
    if (mRuntimeID == -1)
    {
        static_assert(sizeof(union CLIENT_RUNTIME_ID) == sizeof(((CLIENT_RUNTIME_ID*)nullptr)->id), "");

        static int incID = 0;
        incID++;

        CLIENT_RUNTIME_ID tmp;
        tmp.humman.serverID = gSelfID;
        tmp.humman.incID = incID;
        tmp.humman.time = static_cast<int32_t>(time(nullptr));

        mRuntimeID = tmp.id;
        addClientByRuntimeID(shared_from_this(), mRuntimeID);
    }
}

void ClientObject::claimPrimaryServer()
{
    assert(mRuntimeID != -1);
    if (mRuntimeID != -1 && mPrimaryServerID == -1 && !gAllPrimaryServers.empty())
    {
        int randnum = rand() % gAllPrimaryServers.size();
        int i = 0;
        BaseNetSession::PTR    logicServer = nullptr;
        for (auto& v : gAllPrimaryServers)
        {
            if (i++ == randnum)
            {
                mPrimaryServerID = v.first;
                break;
            }
        }

        BigPacket packet(CONNECTION_SERVER_SEND_LOGICSERVER_INIT_CLIENTMIRROR);
        packet.writeINT64(mSocketID);
        packet.writeINT64(mRuntimeID);

        sendPacketToPrimaryServer(packet);
    }
    else if (gAllPrimaryServers.empty())
    {
        gDailyLogger->error("逻辑服务器列表为空");
    }
}