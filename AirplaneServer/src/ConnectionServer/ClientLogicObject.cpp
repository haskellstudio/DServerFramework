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

unordered_map<int64_t, ClientObject::PTR>   gAllClientObject;   //所有的玩家对象,key为运行时ID

ClientObject::PTR findClientByRuntimeID(int64_t runtimeID)
{
    auto it = gAllClientObject.find(runtimeID);
    if (it != gAllClientObject.end())
    {
        return it->second;
    }
    else
    {
        return nullptr;
    }
}

void addClientByRuntimeID(ClientObject::PTR client, int64_t runtimeID)
{
    assert(findClientByRuntimeID(runtimeID) == nullptr);
    gAllClientObject[runtimeID] = client;

    gDailyLogger->warn("add client, runtime id :{}, current online client num:{}", runtimeID, gAllClientObject.size());
}

void eraseClientByRuntimeID(int64_t runtimeID)
{
    gAllClientObject.erase(runtimeID);
    gDailyLogger->warn("remove client, runtime id:{}, current online client num:{}", runtimeID, gAllClientObject.size());
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
    assert(mDelayTimer.lock() == nullptr);  /*已经取消了断线重连定时器*/

    mSocketID = id;
}

int64_t ClientObject::getSocketID() const
{
    return mSocketID;
}

/*当此玩家对象销毁时，通知内部服务器强制退出此RuntimeID标示的玩家--真正退出游戏*/
ClientObject::~ClientObject()
{
    gDailyLogger->info("~ client object : {}", mRuntimeID);
    cancelDelayTimer();

    if (mRuntimeID != -1)
    {
        eraseClientByRuntimeID(mRuntimeID);

        FixedPacket<128> sp;
        sp.setOP(CONNECTION_SERVER_SEND_LOGICSERVER_DESTROY_CLIENT);
        sp.writeINT64(mRuntimeID);
        sp.end();

        sendPacketToPrimaryServer(sp);
        sendPacketToSlaveServer(sp);

        mRuntimeID = -1;
    }
}

void ClientObject::notifyDisConnect()
{
    FixedPacket<128> sp;
    sp.setOP(CONNECTION_SERVER_SEND_LOGICSERVER_CLIENT_DISCONNECT);
    sp.writeINT64(mRuntimeID);
    sp.end();

    sendPacketToPrimaryServer(sp);
}

int64_t ClientObject::getRuntimeID() const
{
    return mRuntimeID;
}

void ClientObject::startDelayWait()
{
    if (!isInDelayWait())
    {
        mSocketID = -1;
        mDelayTimer = gTimerMgr->AddTimer(30000, [=](){
            cout << "等待超时,删除玩家" << endl;
            eraseClientByRuntimeID(mRuntimeID);
        });
    }
}

bool ClientObject::isInDelayWait()
{
    return mDelayTimer.lock() != nullptr;
}

void ClientObject::cancelDelayTimer()
{
    if (mDelayTimer.lock())
    {
        mDelayTimer.lock()->Cancel();
    }
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
        BigPacket packet;
        packet.setOP(CONNECTION_SERVER_SEND_LOGICSERVER_FROMCLIENT);
        packet.writeINT64(mRuntimeID);
        packet.writeBinary(packerBuffer, packetLen);
        packet.end();

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

void ClientObject::setSlaveServerID(int32_t id)
{
    gDailyLogger->info("set client {} slave server id:{}", mRuntimeID, id);
    mSlaveServerID = id;
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

        BigPacket packet;
        packet.setOP(CONNECTION_SERVER_SEND_LOGICSERVER_INIT_CLIENTMIRROR);
        packet.writeINT64(mSocketID);
        packet.writeINT64(mRuntimeID);
        packet.end();

        sendPacketToPrimaryServer(packet);
    }
    else if (gAllPrimaryServers.empty())
    {
        gDailyLogger->error("逻辑服务器列表为空");
    }
}