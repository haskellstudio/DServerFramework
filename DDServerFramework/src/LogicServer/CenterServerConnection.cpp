#include <iostream>
using namespace std;

#include "packet.h"
#include "CenterServerRecvOP.h"
#include "CenterServerSendOP.h"
#include "ConnectionServerConnection.h"
#include "ClientMirror.h"
#include "ConnectionServerRecvOP.h"
#include "WrapLog.h"
#include "AutoConnectionServer.h"
#include "../../ServerConfig/ServerConfig.pb.h"
#include "GlobalValue.h"
#include "CenterServerConnection.h"

std::unordered_map<PACKET_OP_TYPE, CenterServerConnection::USER_MSG_HANDLE>  CenterServerConnection::sUserMsgHandlers;

CenterServerConnection::CenterServerConnection() : BaseLogicSession()
{
    mSelfID = -1;
}

CenterServerConnection::~CenterServerConnection()
{
    if (mPingTimer.lock())
    {
        mPingTimer.lock()->cancel();
    }

    // 开启重连定时器
    gLogicTimerMgr->addTimer(AUTO_CONNECT_DELAY, startConnectThread<UsePacketExtNetSession, CenterServerConnection>, gDailyLogger, gServer, 
        centerServerConfig.enableipv6(), centerServerConfig.bindip(), centerServerConfig.listenport());
}

void CenterServerConnection::registerUserMsgHandle(PACKET_OP_TYPE op, USER_MSG_HANDLE handle)
{
    assert(sUserMsgHandlers.find(op) == sUserMsgHandlers.end());
    sUserMsgHandlers[op] = handle;
}

void CenterServerConnection::onEnter()
{
    gDailyLogger->warn("connect to center server success!");

    TinyPacket sp(static_cast<PACKET_OP_TYPE>(CENTER_SERVER_RECV_OP::CENTERSERVER_RECV_OP_LOGICSERVER_LOGIN));
    sp.writeBinary(centerServerConfig.logicserverloginpassword());
    sp.writeINT32(logicServerConfig.id());

    sendPacket(sp);

    gLogicCenterServerClient = std::static_pointer_cast<CenterServerConnection>(shared_from_this());

    ping();
}

void CenterServerConnection::ping()
{
    TinyPacket p(static_cast<PACKET_OP_TYPE>(CENTER_SERVER_RECV_OP::CENTERSERVER_RECV_OP_PING));

    sendPacket(p);

    mPingTimer = gLogicTimerMgr->addTimer(5000, [shared_this = std::static_pointer_cast<CenterServerConnection>(shared_from_this())](){
        shared_this->ping();
    });
}

void CenterServerConnection::onClose()
{
    gDailyLogger->warn("dis connect to center server!");

    gLogicCenterServerClient = nullptr;
}

void CenterServerConnection::onMsg(const char* data, size_t len)
{
    ReadPacket rp(data, len);
    rp.readPacketLen();
    PACKET_OP_TYPE op = rp.readOP();

    switch (op)
    {
        case CENTERSERVER_SEND_OP_PONG:
        {
        }
        break;
        case CENTERSERVER_SEND_OP_RPC:
        {
            const char* rpcmsg = nullptr;
            size_t len = 0;
            if (rp.readBinary(rpcmsg, len))
            {
                gCenterServerConnectioinRpc.handleRpc(rpcmsg, len);
            }
        }

        break;
        case CENTERSERVER_SEND_OP_PACKET2CLIENTMIRROR:
        {
            onCenterServerSendPacket2Client(rp);
        }
        break;
        case CENTERSERVER_SEND_OP_LOGICSERVER_SELFID:
        {
            mSelfID = rp.readINT32();
        }
        break;
        case CENTERSERVER_SEND_OP_USER:
        {
            onUserMsg(rp);
        }
        break;
        default:
        {
            assert(false);
        }
        break;
    }
}

void CenterServerConnection::onCenterServerSendPacket2Client(ReadPacket& rp)
{
    const char* s = nullptr;
    size_t len = 0;
    if (rp.readBinary(s, len))
    {
        int16_t destNum = rp.readINT16();
        /*  TODO::可以再次组装一个广播消息包,发送给所有的连接服务器 */
        for (int i = 0; i < destNum; ++i)
        {
            int64_t runtimeID = rp.readINT64();
            ClientMirror::PTR user = gClientMirrorMgr->FindClientByRuntimeID(runtimeID);
            if (user != nullptr)
            {
                user->sendPacket(s, len);
            }
        }
    }
}

void CenterServerConnection::onUserMsg(ReadPacket& rp)
{
    const char* s = nullptr;
    size_t len = 0;
    if (rp.readBinary(s, len))
    {
        ReadPacket subPacket(s, len);
        PACKET_LEN_TYPE packetLen = subPacket.readPacketLen();
        assert(packetLen == len);
        if (packetLen == len)
        {
            PACKET_OP_TYPE op = subPacket.readOP();
            auto handleIT = sUserMsgHandlers.find(op);
            if (handleIT != sUserMsgHandlers.end())
            {
                (*handleIT).second(*this, subPacket);
            }
        }
    }
}

void CenterServerConnection::sendPacket(Packet& packet)
{
    send(packet.getData(), packet.getLen());
}

void CenterServerConnection::sendUserPacket(Packet& subPacket)
{
    BigPacket packet(static_cast<PACKET_OP_TYPE>(CENTER_SERVER_RECV_OP::CENTERSERVER_RECV_OP_USER));
    packet.writeBinary(subPacket.getData(), subPacket.getLen());

    sendPacket(packet);
}

int32_t CenterServerConnection::getSelfID() const
{
    return mSelfID;
}