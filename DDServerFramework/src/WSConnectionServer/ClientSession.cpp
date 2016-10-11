#include <unordered_map>

#include "packet.h"
#include "WrapLog.h"
#include "ClientSessionMgr.h"

#include "WebSocketFormat.h"
#include "LogicServerSessionMgr.h"

#include "../../ServerConfig/ServerConfig.pb.h"
#include "./proto/LogicServerWithConnectionServer.pb.h"

#include "ClientSession.h"

extern ServerConfig::ConnectionServerConfig     connectionServerConfig;
extern WrapLog::PTR                             gDailyLogger;
extern WrapServer::PTR                          gServer;
static std::atomic<int32_t> incRuntimeID = ATOMIC_VAR_INIT(0);

ConnectionClientSession::ConnectionClientSession()
{
    mRuntimeID = -1;
    mPrimaryServerID = -1;
    mSlaveServerID = -1;

    mRecvSerialID = 0;
    mSendSerialID = 0;

    gDailyLogger->info("ConnectionClientSession : {}, {}", getSocketID(), getIP());
}

ConnectionClientSession::~ConnectionClientSession()
{
    gDailyLogger->info("~ ConnectionClientSession : {}, {}", getSocketID(), getIP());
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

void ConnectionClientSession::claimRuntimeID()
{
    if (mRuntimeID == -1)
    {
        static_assert(sizeof(union CLIENT_RUNTIME_ID) == sizeof(((CLIENT_RUNTIME_ID*)nullptr)->id), "");

        auto tmpRuntimeID = std::atomic_fetch_add(&incRuntimeID, 1) + 1;

        CLIENT_RUNTIME_ID tmp;
        tmp.humman.serverID = connectionServerConfig.id();
        tmp.humman.incID = tmpRuntimeID;
        tmp.humman.time = static_cast<int32_t>(time(nullptr));

        mRuntimeID = tmp.id;
        ClientSessionMgr::AddClientByRuntimeID(std::static_pointer_cast<ConnectionClientSession>(shared_from_this()), mRuntimeID);
    }
}

void ConnectionClientSession::claimPrimaryServer()
{
    assert(mRuntimeID != -1);
    if (mRuntimeID != -1 && mPrimaryServer == nullptr)
    {
        mPrimaryServerID = LogicServerSessionMgr::ClaimPrimaryLogicServer();
        if (mPrimaryServerID != -1)
        {
            mPrimaryServer = LogicServerSessionMgr::FindPrimaryLogicServer(mPrimaryServerID);
        }
        else
        {
            gDailyLogger->error("分配主逻辑服务器失败");
        }
    }
}

int ConnectionClientSession::getPrimaryServerID() const
{
    return mPrimaryServerID;
}

int64_t ConnectionClientSession::getRuntimeID() const
{
    return mRuntimeID;
}

void ConnectionClientSession::sendPBBinary(int32_t cmd, const char* data, size_t len)
{
    auto tmp = std::make_shared<std::string>(data, len);

    getEventLoop()->pushAsyncProc([=](){

        char b[8 * 1024];
        BasePacketWriter packet(b, sizeof(b), true);
        packet.writeINT8('{');
        packet.writeUINT32(tmp->size() + 14);
        packet.writeUINT32(cmd);
        packet.writeUINT32(mSendSerialID);
        packet.writeBuffer(tmp->c_str(), tmp->size());
        packet.writeINT8('}');

        std::string frame;
        if (WebSocketFormat::wsFrameBuild(std::string(packet.getData(), packet.getPos()), frame))
        {
            sendPacket(frame.c_str(), frame.size());
            mSendSerialID++;
        }
        else
        {
            gDailyLogger->error("wsFrameBuild failed of client runtime id :{}", mRuntimeID);
        }
    });
}

void ConnectionClientSession::setSlaveServerID(int id)
{
    gDailyLogger->info("set client {} slave server id:{}", mRuntimeID, id);
    mSlaveServerID = id;
}

void ConnectionClientSession::procPacket(uint32_t op, const char* body, uint32_t bodyLen)
{
    if (mRuntimeID == -1)
    {
        claimRuntimeID();
    }

    if (mPrimaryServerID == -1)
    {
        claimPrimaryServer();
    }

    if (mSlaveServerID != -1)
    {
        if (mSlaveServer == nullptr)
        {
            mSlaveServer = LogicServerSessionMgr::FindSlaveLogicServer(mSlaveServerID);
        }
    }
    else
    {
        mSlaveServer = nullptr;
    }

    internalAgreement::UpstreamACK upstream;
    upstream.set_clientid(mRuntimeID);
    upstream.set_msgid(op);
    upstream.set_data(body, bodyLen);

    const int UPSTREAM_ACK_OP = 2210821449;

    if (op == 24)   /*  特定消息固定转发到玩家所属主逻辑服务器(TODO::常量和OP枚举定义) */
    {
        if (mPrimaryServer != nullptr)
        {
            mPrimaryServer->sendPB(UPSTREAM_ACK_OP, upstream);
        }
    }
    else
    {
        if (mSlaveServer != nullptr)
        {
            mSlaveServer->sendPB(UPSTREAM_ACK_OP, upstream);
        }
        else if (mPrimaryServer != nullptr)
        {
            mPrimaryServer->sendPB(UPSTREAM_ACK_OP, upstream);
        }
    }
}

void ConnectionClientSession::onEnter()
{
    gDailyLogger->warn("client enter, ip:{}, socket id :{}", getIP(), getSocketID());
}

void ConnectionClientSession::onClose()
{
    gDailyLogger->warn("client close, ip:{}, socket id :{}, runtime id:{}", getIP(), getSocketID(), getRuntimeID());
    if (mRuntimeID != -1)
    {
        ClientSessionMgr::EraseClientByRuntimeID(mRuntimeID);

        internalAgreement::CloseClientACK closeAck;
        closeAck.set_clientid(mRuntimeID);

        const int CLOSE_CLIENT_ACK_OP = 1513370829;

        if (mPrimaryServer != nullptr)
        {
            mPrimaryServer->sendPB(CLOSE_CLIENT_ACK_OP, closeAck);
        }
        if (mSlaveServer != nullptr)
        {
            mSlaveServer->sendPB(CLOSE_CLIENT_ACK_OP, closeAck);
        }

        mRuntimeID = -1;
    }
}