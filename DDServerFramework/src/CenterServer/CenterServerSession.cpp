#include <iostream>

using namespace std;

#include "packet.h"
#include "CenterServerRecvOP.h"
#include "CenterServerSendOP.h"
#include "WrapLog.h"
#include "../../ServerConfig/ServerConfig.pb.h"

#include "CenterServerSession.h"

unordered_map<int, CenterServerSession::PTR> CenterServerSessionGlobalData::sAllLogicServer;
std::shared_ptr<dodo::rpc::RpcService < dodo::rpc::MsgpackProtocol>> CenterServerSessionGlobalData::sCenterServerSessionRpc;
CenterServerSession::PTR CenterServerSessionGlobalData::sCenterServerSessionRpcFromer;

extern WrapLog::PTR gDailyLogger;
extern ServerConfig::CenterServerConfig centerServerConfig;

std::unordered_map<PACKET_OP_TYPE, CenterServerSession::USER_MSG_HANDLE>   CenterServerSessionGlobalData::sUserMsgHandles;

void CenterServerSessionGlobalData::registerUserMsgHandle(PACKET_OP_TYPE op, CenterServerSession::USER_MSG_HANDLE handle)
{
    assert(sUserMsgHandles.find(op) == sUserMsgHandles.end());
    sUserMsgHandles[op] = handle;
}

CenterServerSession::USER_MSG_HANDLE CenterServerSessionGlobalData::findUserMsgHandle(PACKET_OP_TYPE op)
{
    auto it = sUserMsgHandles.find(op);
    if (it != sUserMsgHandles.end())
    {
        return (*it).second;
    }

    return nullptr;
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
        CenterServerSessionGlobalData::removeLogicServer(mID);
    }
}

void CenterServerSession::onMsg(const char* data, size_t len)
{
    ReadPacket rp(data, len);
    rp.readPacketLen();
    CENTER_SERVER_RECV_OP op = static_cast<CENTER_SERVER_RECV_OP>(rp.readOP());

    gDailyLogger->info("recv {} ,op :{}", getIP(), static_cast<PACKET_OP_TYPE>(op));

    switch (op)
    {
        case CENTER_SERVER_RECV_OP::CENTERSERVER_RECV_OP_PING:
            onPing(rp);
            break;
        case CENTER_SERVER_RECV_OP::CENTERSERVER_RECV_OP_RPC:
            onLogicServerRpc(rp);
            break;
        case CENTER_SERVER_RECV_OP::CENTERSERVER_RECV_OP_LOGICSERVER_LOGIN:
            onLogicServerLogin(rp);
            break;
        case CENTER_SERVER_RECV_OP::CENTERSERVER_RECV_OP_USER:
            onUserMsg(rp);
            break;
        default:
            gDailyLogger->error("recv error op :{}", static_cast<PACKET_OP_TYPE>(op));
            break;
    }

    if (rp.getMaxPos() != rp.getPos())
    {
        gDailyLogger->error("packet parse error, {}/{}", rp.getPos(), rp.getMaxPos());
        rp.skipAll();
    }
}

int CenterServerSession::getID() const
{
    return mID;
}

void    CenterServerSession::sendPacket(Packet& packet)
{
    send(packet.getData(), packet.getLen());
}

void CenterServerSession::sendUserPacket(Packet& subPacket)
{
    BigPacket packet(CENTERSERVER_SEND_OP_USER);
    packet.writeBinary(subPacket.getData(), subPacket.getLen());

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
        CenterServerSessionGlobalData::setRpcFrommer(std::static_pointer_cast<CenterServerSession>(shared_from_this()));
        CenterServerSessionGlobalData::getCenterServerSessionRpc()->handleRpc(msg, len);
    }
}

void CenterServerSession::onLogicServerLogin(ReadPacket& rp)
{
    gDailyLogger->info("内部服务器登陆");

    if (mID == -1)
    {
        string password = rp.readBinary();
        int32_t id = rp.readINT32();

        if (password == centerServerConfig.logicserverloginpassword())
        {
            if (CenterServerSessionGlobalData::findLogicServer(id) == nullptr)
            {
                gDailyLogger->info("id 为:{}", id);

                mID = id;

                BigPacket sp(CENTERSERVER_SEND_OP_LOGICSERVER_SELFID);
                sp.writeINT32(mID);
                sendPacket(sp);

                CenterServerSessionGlobalData::insertLogicServer(std::static_pointer_cast<CenterServerSession>(shared_from_this()), mID);
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
    BigPacket packet(CENTERSERVER_SEND_OP_PACKET2CLIENTMIRROR);
    packet.writeBinary(realPacket.getData(), realPacket.getLen());
    packet.writeINT16(1);
    packet.writeINT64(runtimeID);

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

        auto handle = CenterServerSessionGlobalData::findUserMsgHandle(subOP);
        if (handle != nullptr)
        {
            handle(std::static_pointer_cast<CenterServerSession>(shared_from_this()), subRP);
        }
        else
        {
            gDailyLogger->info("recv not register op , op :{}", getIP(), subOP);
        }
    }
}

void CenterServerSessionGlobalData::init()
{
    sCenterServerSessionRpc = std::make_shared<dodo::rpc::RpcService < dodo::rpc::MsgpackProtocol>>();
}

void CenterServerSessionGlobalData::destroy()
{
    sAllLogicServer.clear();
    sCenterServerSessionRpc = nullptr;
    sCenterServerSessionRpcFromer = nullptr;
    sUserMsgHandles.clear();
}

CenterServerSession::PTR CenterServerSessionGlobalData::findLogicServer(int id)
{
    auto it = sAllLogicServer.find(id);
    if (it != sAllLogicServer.end())
    {
        return (*it).second;
    }

    return nullptr;
}

void CenterServerSessionGlobalData::removeLogicServer(int id)
{
    sAllLogicServer.erase(id);
}

void CenterServerSessionGlobalData::insertLogicServer(CenterServerSession::PTR logicServer, int id)
{
    sAllLogicServer[id] = logicServer;
}

CenterServerSession::PTR& CenterServerSessionGlobalData::getRpcFromer()
{
    return sCenterServerSessionRpcFromer;
}

void CenterServerSessionGlobalData::setRpcFrommer(CenterServerSession::PTR fromer)
{
    sCenterServerSessionRpcFromer = fromer;
}

std::shared_ptr<dodo::rpc::RpcService < dodo::rpc::MsgpackProtocol>>& CenterServerSessionGlobalData::getCenterServerSessionRpc()
{
    return sCenterServerSessionRpc;
}