#include <iostream>

#include "packet.h"
#include "CenterServerRecvOP.h"
#include "CenterServerSendOP.h"

#include "SSDBMultiClient.h"

#include "WrapLog.h"

#include "CenterServerPassword.h"
#include "CenterServerSession.h"

using namespace std;

extern  SSDBMultiClient::PTR                    gSSDBProcyClient;
unordered_map<int, CenterServerSession*>        gAllLogicServer;
unordered_map<int, CenterServerSession*>        gAllGameServer;
unordered_map<int, std::pair<string, int>>      gAllconnectionservers;
dodo::rpc < dodo::MsgpackProtocol>              gCenterServerSessionRpc;

extern WrapLog::PTR gDailyLogger;

std::unordered_map<PACKET_OP_TYPE, CenterServerSession::USER_MSG_HANDLE>   CenterServerSession::sUserMsgHandles;

void CenterServerSession::registerUserMsgHandle(PACKET_OP_TYPE op, USER_MSG_HANDLE handle)
{
    assert(sUserMsgHandles.find(op) == sUserMsgHandles.end());
    sUserMsgHandles[op] = handle;
}

CenterServerSession::CenterServerSession() : BaseLogicSession()
{
    mID = -1;
}

void CenterServerSession::onEnter()
{
}

void CenterServerSession::onClose()
{
    if (mID != -1)
    {
        gDailyLogger->warn("内部服务器断开, id: {}", mID);
        gAllLogicServer.erase(mID);
    }
}

void CenterServerSession::onMsg(const char* data, size_t len)
{
    ReadPacket rp(data, len);
    rp.readPacketLen();
    PACKET_OP_TYPE op = rp.readOP();

    gDailyLogger->info("recv {} ,op :{}", getIP(), op);

    switch (op)
    {
        case CENTERSERVER_RECV_OP_PING:
            onPing(rp);
            break;
        case CENTERSERVER_RECV_OP_RPC:
            onLogicServerRpc(rp);
            break;
        case CENTERSERVER_RECV_OP_LOGICSERVER_LOGIN:
            onLogicServerLogin(rp);
            break;
        case CENTERSERVER_RECV_OP_USER:
            onUserMsg(rp);
            break;
        default:
            gDailyLogger->error("recv error op :{}", op);
            break;
    }

    if (rp.getMaxPos() != rp.getPos())
    {
        gDailyLogger->error("packet parse error, {}/{}", rp.getPos(), rp.getMaxPos());
        rp.skipAll();
    }
}

void    CenterServerSession::sendPacket(Packet& packet)
{
    send(packet.getData(), packet.getLen());
}

void CenterServerSession::sendUserPacket(Packet& subPacket)
{
    BigPacket packet;
    packet.setOP(CENTERSERVER_SEND_OP_USER);
    packet.writeBinary(subPacket.getData(), subPacket.getLen());
    packet.end();

    sendPacket(packet);
}

void CenterServerSession::onPing(ReadPacket& rp)
{
    gDailyLogger->info("recv ping from logic server, id:{}", mID);
    sendv(CENTERSERVER_SEND_OP_PONG);
}

void CenterServerSession::onLogicServerRpc(ReadPacket& rp)
{
    const char* msg = nullptr;
    size_t len = 0;
    if (rp.readBinary(msg, len))
    {
        gCenterServerSessionRpc.handleRpc(msg, len);
    }
}

void CenterServerSession::onLogicServerLogin(ReadPacket& rp)
{
    gDailyLogger->info("内部服务器登陆");

    if (mID == -1)
    {
        string password = rp.readBinary();
        int32_t id = rp.readINT32();

        if (password == CenterServerPassword::getInstance().getPassword())
        {
            if (gAllLogicServer.find(id) == gAllLogicServer.end())
            {
                gDailyLogger->info("id 为:{}", id);

                mID = id;

                BigPacket sp;
                sp.setOP(CENTERSERVER_SEND_OP_LOGICSERVER_SELFID);
                sp.writeINT32(mID);
                sp.end();
                sendPacket(sp);

                gAllLogicServer[mID] = this;
            }
            else
            {
                gDailyLogger->error("recv logic server, id:{} 重复", id);
            }
        }
        else
        {
            gDailyLogger->error("recv logic server登陆失败，密码错误");
        }
    }
}

void CenterServerSession::sendPacket2Client(int64_t runtimeID, Packet& realPacket)
{
    BigPacket packet;
    packet.setOP(CENTERSERVER_SEND_OP_PACKET2CLIENTMIRROR);
    packet.writeBinary(realPacket.getData(), realPacket.getLen());
    packet.writeINT16(1);
    packet.writeINT64(runtimeID);
    packet.end();

    sendPacket(packet);
}

void CenterServerSession::onUserMsg(ReadPacket& rp)
{
    const char* subPacket;
    size_t subPacketLen;
    if (rp.readBinary(subPacket, subPacketLen))
    {
        ReadPacket subRP(subPacket, subPacketLen);
        subRP.readPacketLen();
        PACKET_OP_TYPE subOP = subRP.readOP();

        auto handleIT = sUserMsgHandles.find(subOP);
        if (handleIT != sUserMsgHandles.end())
        {
            (*handleIT).second(*this, subRP);
        }
        else
        {
            gDailyLogger->info("recv not register op , op :{}", getIP(), subOP);
        }
    }
}