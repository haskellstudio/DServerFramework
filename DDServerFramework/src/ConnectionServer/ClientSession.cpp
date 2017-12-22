#include <unordered_map>

#include <brynet/utils/packet.h>
#include <brynet/net/http/WebSocketFormat.h>

#include "WrapLog.h"
#include "ClientSessionMgr.h"
#include "ConnectionServerSendOP.h"
#include "LogicServerSessionMgr.h"

#include "../../ServerConfig/ServerConfig.pb.h"

#include "ClientSession.h"

extern WrapLog::PTR                             gDailyLogger;
static std::atomic<int32_t> incRuntimeID = ATOMIC_VAR_INIT(0);

ConnectionClientSession::ConnectionClientSession(int32_t connectionServerID,
    int64_t socketID, std::string ip) 
    : mConnectionServerID(connectionServerID),
    mSocketID(socketID),
    mIP(ip)

{
    mRuntimeID = -1;
    mPrimaryServerID = -1;
    mSlaveServerID = -1;

    mRecvSerialID = 0;
    mSendSerialID = 0;
    mReconnecting = false;
}

ConnectionClientSession::~ConnectionClientSession()
{
}

static int64_t MakeRuntimeID(int32_t csID)
{
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

    static_assert(sizeof(union CLIENT_RUNTIME_ID) == sizeof(((CLIENT_RUNTIME_ID*)nullptr)->id), "");

    static std::atomic<int32_t> incRuntimeID = ATOMIC_VAR_INIT(0);

    CLIENT_RUNTIME_ID tmp;
    tmp.humman.serverID = csID;
    tmp.humman.incID = std::atomic_fetch_add(&incRuntimeID, 1);
    tmp.humman.time = static_cast<int32_t>(time(nullptr));

    return tmp.id;
}

void ConnectionClientSession::claimPrimaryServer()
{
    assert(mRuntimeID != -1);
    auto server = std::atomic_load(&mPrimaryServer);
    if (mRuntimeID == -1 || server != nullptr)
    {
        return;
    }

    auto primaryID = LogicServerSessionMgr::ClaimPrimaryLogicServer();
    if (primaryID != -1)
    {
        std::atomic_store(&mPrimaryServer, LogicServerSessionMgr::FindPrimaryLogicServer(primaryID));
    }
    else
    {
        gDailyLogger->error("claim primary server failed of runtime id :{}, ip:{}", mRuntimeID, getIP());
    }
}

void ConnectionClientSession::setPrimaryServerID(int id)
{
    gDailyLogger->info("set client {} primary server id:{}", mRuntimeID, id);

    LogicServerSession::PTR server = nullptr;
    if (id != -1)
    {
        server = LogicServerSessionMgr::FindPrimaryLogicServer(id);
        mReconnecting = false;
    }
    std::atomic_store(&mPrimaryServer, server);
}

int ConnectionClientSession::getPrimaryServerID() const
{
    auto server = std::atomic_load(&mPrimaryServer);
    return server ? server->getID() : -1;
}

void ConnectionClientSession::setSlaveServerID(int id)
{
    gDailyLogger->info("set client {} slave server id:{}", mRuntimeID, id);

    LogicServerSession::PTR server = nullptr;
    if (id != -1)
    {
        server = LogicServerSessionMgr::FindSlaveLogicServer(id);
    }
    std::atomic_store(&mSlaveServer, server);
}

int ConnectionClientSession::getSlaveServerID() const
{
    return mSlaveServerID;
}

int64_t ConnectionClientSession::getRuntimeID() const
{
    return mRuntimeID;
}

void ConnectionClientSession::notifyExit()
{
    gDailyLogger->info("ready notify client {} exist game", mRuntimeID);

    TinyPacket sp(static_cast<PACKET_OP_TYPE>(CONNECTION_SERVER_SEND_OP::CONNECTION_SERVER_SEND_LOGICSERVER_DESTROY_CLIENT));
    sp.writeINT64(mRuntimeID);
    sp.getLen();

    // 如果没有找到所属服务器对象,但玩家正在重连中,那么需要向所有服务器发送断开(避免后台已经重连成功,然后此链接断开,后面再提出此链接就会失败)

    auto server = std::atomic_load(&mPrimaryServer);
    if (server != nullptr)
    {
        gDailyLogger->info("really notify client {} exist primary server", mRuntimeID);
        server->send(sp.getData(), sp.getLen());
    }
    else if (mReconnecting)
    {
        auto primaryServers = LogicServerSessionMgr::GetAllPrimaryLogicServer();
        for (auto& v : primaryServers)
        {
            v.second->send(sp.getData(), sp.getLen());
        }
    }

    server = std::atomic_load(&mSlaveServer);
    if (server != nullptr)
    {
        gDailyLogger->info("really notify client {} exist salve server", mRuntimeID);
        server->send(sp.getData(), sp.getLen());
    }
    else if (mReconnecting)
    {
        auto slaveServers = LogicServerSessionMgr::GetAllSlaveLogicServer();
        for (auto& v : slaveServers)
        {
            v.second->send(sp.getData(), sp.getLen());
        }
    }
}

void ConnectionClientSession::procPacket(PACKET_OP_TYPE op, const char* body, PACKET_LEN_TYPE bodyLen)
{
    // TODO::交给后端逻辑服务器配置
    const static std::vector<uint32_t> PRIMARY_OPS = {};

    mRecvSerialID++;

    gDailyLogger->info("recv op {} from id:{}", op, mRuntimeID);

    BigPacket packet(static_cast<PACKET_OP_TYPE>(CONNECTION_SERVER_SEND_OP::CONNECTION_SERVER_SEND_LOGICSERVER_FROMCLIENT));
    packet.writeINT64(mSocketID);
    packet.writeINT64(mRuntimeID);
    packet.writeBinary(body - PACKET_HEAD_LEN, bodyLen + PACKET_HEAD_LEN);
    packet.getLen();

    // TODO::重连消息包
    if (op == 91)
    {
        /*  重连包必须为第一个消息包    */
        if (mRecvSerialID == 1)
        {
            auto primaryServers = LogicServerSessionMgr::GetAllPrimaryLogicServer();
            for (auto& server : primaryServers)
            {
                server.second->send(packet.getData(), packet.getLen());
            }
        }

        mReconnecting = true;

        return;
    }

    auto primaryServer = std::atomic_load(&mPrimaryServer);
    auto slaveServer = std::atomic_load(&mSlaveServer);

    if (!primaryServer && !mReconnecting)
    {
        /*  只有此链接没有请求过重连才主动给他分配主服务器 */
        claimPrimaryServer();
        primaryServer = std::atomic_load(&mPrimaryServer);
    }

    if (std::find(PRIMARY_OPS.begin(), PRIMARY_OPS.end(), op) != PRIMARY_OPS.end())   /*  特定消息固定转发到玩家所属主逻辑服务器(TODO::常量和OP枚举定义) */
    {
        if (primaryServer != nullptr)
        {
            primaryServer->send(packet.getData(), packet.getLen());
        }
        else
        {
            gDailyLogger->error("primary server is nil");
        }
    }
    else
    {
        if (slaveServer != nullptr)
        {
            slaveServer->send(packet.getData(), packet.getLen());
        }
        else if (primaryServer != nullptr)
        {
            primaryServer->send(packet.getData(), packet.getLen());
        }
        else
        {
            gDailyLogger->error("no any server process this packet cmd:{} of client {}, ip is {}", op, mRuntimeID, getIP());
        }
    }
}

int64_t ConnectionClientSession::getSocketID() const
{
    return mSocketID;
}

const std::string& ConnectionClientSession::getIP() const
{
    return mIP;
}

void ConnectionClientSession::onEnter()
{
    mRuntimeID = MakeRuntimeID(mConnectionServerID);
    ClientSessionMgr::AddClientByRuntimeID(std::static_pointer_cast<ConnectionClientSession>(shared_from_this()), mRuntimeID);
    gDailyLogger->warn("client enter, ip:{}, socket id :{}", getIP(), getSocketID());
}

void ConnectionClientSession::onClose()
{
    gDailyLogger->info("client close, ip:{}, socket id :{}, runtime id:{}", getIP(), getSocketID(), getRuntimeID());
    if (mRuntimeID != -1)
    {
        auto sharedThis = std::static_pointer_cast<ConnectionClientSession>(shared_from_this());
        getEventLoop()->getTimerMgr()->addTimer(std::chrono::milliseconds(2 * 60 * 1000), [=]() {
            sharedThis->notifyExit();
            ClientSessionMgr::EraseClientByRuntimeID(sharedThis->getRuntimeID());
            //不在onClose里调用立即EraseClientByRuntimeID，是因为有可能玩家另外一个链接再次登录时，先前的链接先触发网络断开,然后后续链接再触发kick，那么网络断开里移除了玩家，并且等待2分钟再通知退出
            //那么后续的kick逻辑里就无法触发通知玩家离线(则玩家无法短时间内再次登录)(因为找不到玩家对象了)
        });
    }
}