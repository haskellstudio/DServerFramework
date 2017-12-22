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

    // ���û���ҵ���������������,���������������,��ô��Ҫ�����з��������ͶϿ�(�����̨�Ѿ������ɹ�,Ȼ������ӶϿ�,��������������Ӿͻ�ʧ��)

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
    // TODO::��������߼�����������
    const static std::vector<uint32_t> PRIMARY_OPS = {};

    mRecvSerialID++;

    gDailyLogger->info("recv op {} from id:{}", op, mRuntimeID);

    BigPacket packet(static_cast<PACKET_OP_TYPE>(CONNECTION_SERVER_SEND_OP::CONNECTION_SERVER_SEND_LOGICSERVER_FROMCLIENT));
    packet.writeINT64(mSocketID);
    packet.writeINT64(mRuntimeID);
    packet.writeBinary(body - PACKET_HEAD_LEN, bodyLen + PACKET_HEAD_LEN);
    packet.getLen();

    // TODO::������Ϣ��
    if (op == 91)
    {
        /*  ����������Ϊ��һ����Ϣ��    */
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
        /*  ֻ�д�����û��������������������������������� */
        claimPrimaryServer();
        primaryServer = std::atomic_load(&mPrimaryServer);
    }

    if (std::find(PRIMARY_OPS.begin(), PRIMARY_OPS.end(), op) != PRIMARY_OPS.end())   /*  �ض���Ϣ�̶�ת��������������߼�������(TODO::������OPö�ٶ���) */
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
            //����onClose���������EraseClientByRuntimeID������Ϊ�п����������һ�������ٴε�¼ʱ����ǰ�������ȴ�������Ͽ�,Ȼ����������ٴ���kick����ô����Ͽ����Ƴ�����ң����ҵȴ�2������֪ͨ�˳�
            //��ô������kick�߼�����޷�����֪ͨ�������(������޷���ʱ�����ٴε�¼)(��Ϊ�Ҳ�����Ҷ�����)
        });
    }
}