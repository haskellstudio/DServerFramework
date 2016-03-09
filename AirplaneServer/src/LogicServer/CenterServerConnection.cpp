#include <iostream>
using namespace std;

#include "socketlibfunction.h"
#include "packet.h"
#include "CenterServerRecvOP.h"
#include "CenterServerSendOP.h"
#include "NetThreadSession.h"
#include "ConnectionServerConnection.h"
#include "ClientMirror.h"
#include "ConnectionServerRecvOP.h"
#include "WrapLog.h"
#include "UsePacketExtNetSession.h"
#include "AutoConnectionServer.h"
#include "CenterServerConnection.h"
#include "CenterServerPassword.h"

extern ClientMirrorMgr::PTR   gClientMirrorMgr;
extern TimerMgr::PTR   gTimerMgr;
extern int gSelfID;
extern WrapServer::PTR   gServer;
extern WrapLog::PTR  gDailyLogger;
extern string centerServerIP;
extern int centerServerPort;

CenterServerConnection* gLogicCenterServerClient = nullptr;
dodo::rpc<dodo::MsgpackProtocol>    gCenterServerConnectioinRpc;
std::unordered_map<PACKET_OP_TYPE, CenterServerConnection::USER_MSG_HANDLE>  CenterServerConnection::sUserMsgHandlers;

CenterServerConnection::CenterServerConnection() : BaseLogicSession()
{
    mSelfID = -1;
}

CenterServerConnection::~CenterServerConnection()
{
    if (mPingTimer.lock())
    {
        mPingTimer.lock()->Cancel();
    }

    gTimerMgr->AddTimer(AUTO_CONNECT_DELAY, startConnectThread<UsePacketExtNetSession, CenterServerConnection>, gDailyLogger, gServer, centerServerIP, centerServerPort);
}

void CenterServerConnection::registerUserMsgHandle(PACKET_OP_TYPE op, USER_MSG_HANDLE handle)
{
    assert(sUserMsgHandlers.find(op) == sUserMsgHandlers.end());
    sUserMsgHandlers[op] = handle;
}

void CenterServerConnection::onEnter()
{
    gDailyLogger->warn("connect to center server success!");

    FixedPacket<128> sp;
    sp.setOP(CENTERSERVER_RECV_OP_LOGICSERVER_LOGIN);
    sp.writeBinary(CenterServerPassword::getInstance().getPassword());
    sp.writeINT32(gSelfID);
    sp.end();

    sendPacket(sp);

    gLogicCenterServerClient = this;

    ping();
}

void CenterServerConnection::ping()
{
    FixedPacket<128> p;
    p.setOP(CENTERSERVER_RECV_OP_PING);
    p.end();

    sendPacket(p);

    mPingTimer = gTimerMgr->AddTimer(5000, [this](){
        ping();
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
            ClientMirror* p = gClientMirrorMgr->FindClientByRuntimeID(runtimeID);
            if (p != nullptr)
            {
                p->sendPacket(s, len);
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
    BigPacket packet;
    packet.setOP(CENTERSERVER_RECV_OP_USER);
    packet.writeBinary(subPacket.getData(), subPacket.getLen());
    packet.end();

    sendPacket(packet);
}

int32_t CenterServerConnection::getSelfID() const
{
    return mSelfID;
}