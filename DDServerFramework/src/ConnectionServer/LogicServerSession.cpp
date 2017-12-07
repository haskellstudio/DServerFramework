#include <unordered_map>
#include <set>

#include <brynet/utils/packet.h>
#include <brynet/net/DataSocket.h>

#include "ConnectionServerSendOP.h"
#include "ConnectionServerRecvOP.h"
#include "ClientSession.h"
#include "WrapLog.h"

#include "../../ServerConfig/ServerConfig.pb.h"

#include "ClientSessionMgr.h"
#include "LogicServerSessionMgr.h"

#include "LogicServerSession.h"

using namespace brynet::net;
extern WrapLog::PTR                         gDailyLogger;

LogicServerSession::LogicServerSession(int32_t connectionServerID, std::string password) :
    mConnectionServerID(connectionServerID), mPassword(password)
{
    mIsPrimary = false;
    mID = -1;
}

void LogicServerSession::onEnter()
{
    gDailyLogger->warn("recv logic server enter");
}

void LogicServerSession::onClose()
{
    gDailyLogger->warn("recv logic server dis connect, server id : {}.", mID);

    if (mID != -1)
    {
        if (mIsPrimary)
        {
            ClientSessionMgr::KickClientOfPrimary(mID);
            LogicServerSessionMgr::RemovePrimaryLogicServer(mID);
        }
        else
        {
            LogicServerSessionMgr::RemoveSlaveLogicServer(mID);
        }

        mIsPrimary = false;
        mID = -1;
    }
}

int LogicServerSession::getID() const
{
    return mID;
}

bool LogicServerSession::checkPassword(const std::string& password)
{
    return password == mPassword;
}

void LogicServerSession::sendLogicServerLoginResult(bool isSuccess, const std::string& reason)
{
    TinyPacket sp(static_cast<PACKET_OP_TYPE>(CONNECTION_SERVER_SEND_OP::CONNECTION_SERVER_SEND_LOGICSERVER_RECVCSID));
    sp.writeINT32(mConnectionServerID);
    sp.writeBool(isSuccess);
    sp.writeBinary(reason);
    send(sp.getData(), sp.getLen());
}

void LogicServerSession::procPacket(PACKET_OP_TYPE op, const char* body, PACKET_LEN_TYPE bodyLen)
{
    gDailyLogger->debug("recv logic server packet, op:{}, bodylen:{}", op, bodyLen);

    ReadPacket rp(body, bodyLen);
    switch (op)
    {
    case CONNECTION_SERVER_RECV_PING:
    {
        onPing(rp);
    }
    break;
    case CONNECTION_SERVER_RECV_LOGICSERVER_LOGIN:
    {
        onLogicServerLogin(rp);
    }
    break;
    case CONNECTION_SERVER_RECV_PACKET2CLIENT_BYSOCKINFO:
    {
        onPacket2ClientBySocketInfo(rp);
    }
    break;
    case CONNECTION_SERVER_RECV_PACKET2CLIENT_BYRUNTIMEID:
    {
        onPacket2ClientByRuntimeID(rp);
    }
    break;
    case CONNECTION_SERVER_RECV_IS_SETCLIENT_SLAVEID:
    {
        onSlaveServerIsSetClient(rp);
    }
    break;
    case CONNECTION_SERVER_RECV_IS_SETCLIENT_PRIMARYID:
    {
        onPrimaryServerIsSetClient(rp);
    }
    break;
    case CONNECTION_SERVER_RECV_KICKCLIENT_BYRUNTIMEID:
    {
        onKickClientByRuntimeID(rp);
    }
    break;
    default:
    {
        assert(false);
    }
    break;
    }
}

void LogicServerSession::onLogicServerLogin(ReadPacket& rp)
{
    std::string password = rp.readBinary();
    int id = rp.readINT32();
    bool isPrimary = rp.readBool();

    gDailyLogger->info("收到逻辑服务器登陆, ID:{},密码:{}", id, password);

    bool loginResult = false;
    std::string reason;

    if (checkPassword(password))
    {
        bool isSuccess = false;

        if (isPrimary)
        {
            if (LogicServerSessionMgr::FindPrimaryLogicServer(id) == nullptr)
            {
                LogicServerSessionMgr::AddPrimaryLogicServer(id, std::static_pointer_cast<LogicServerSession>(shared_from_this()));
                isSuccess = true;
            }
        }
        else
        {
            if (LogicServerSessionMgr::FindSlaveLogicServer(id) == nullptr)
            {
                LogicServerSessionMgr::AddSlaveLogicServer(id, std::static_pointer_cast<LogicServerSession>(shared_from_this()));
                isSuccess = true;
            }
        }

        if (isSuccess)
        {
            mIsPrimary = isPrimary;
            loginResult = true;
            mID = id;
        }
        else
        {
            reason = "ID对应的Logic Server已存在";
        }
    }
    else
    {
        reason = "密码错误";
    }

    sendLogicServerLoginResult(loginResult, reason);
}

const static bool IsPrintPacketSendedLog = true;

void LogicServerSession::onPacket2ClientBySocketInfo(ReadPacket& rp)
{
    const char* realPacketBuff = nullptr;
    size_t realPacketLen = 0;
    if (!rp.readBinary(realPacketBuff, realPacketLen))
    {
        return;
    }

    int16_t destNum = rp.readINT16();

    DataSocket::PACKET_PTR p = DataSocket::makePacket(realPacketBuff, realPacketLen);

    ReadPacket realReadpacket(realPacketBuff, realPacketLen);
    realPacketLen = realReadpacket.readPacketLen();
    auto realOP = realReadpacket.readOP();
    realReadpacket.skipAll();

    for (int i = 0; i < destNum; ++i)
    {
        int32_t csID = rp.readINT32();
        int64_t clientSocketID = rp.readINT64();

        if (csID != mConnectionServerID)
        {
            continue;
        }

        if (IsPrintPacketSendedLog)
        {
            getService()->getService()->send(clientSocketID, p, std::make_shared<DataSocket::PACKED_SENDED_CALLBACK::element_type>([clientSocketID, realOP, realPacketLen]() {
                gDailyLogger->info("send complete to {}, op:{}, len:{}", clientSocketID, realOP, realPacketLen);
            }));
        }
        else
        {
            getService()->getService()->send(clientSocketID, p);
        }
    }
}

void LogicServerSession::onPacket2ClientByRuntimeID(ReadPacket& rp)
{
    const char* realPacketBuff = nullptr;
    size_t realPacketLen = 0;
    if (!rp.readBinary(realPacketBuff, realPacketLen))
    {
        return;
    }

    int16_t destNum = rp.readINT16();

    ReadPacket realReadpacket(realPacketBuff, realPacketLen);
    realPacketLen = realReadpacket.readPacketLen();
    PACKET_OP_TYPE realOP = realReadpacket.readOP();
    realReadpacket.skipAll();

    DataSocket::PACKET_PTR p = DataSocket::makePacket(realPacketBuff, realPacketLen);

    for (int i = 0; i < destNum; ++i)
    {
        int64_t runtimeID = rp.readINT64();
        auto client = ClientSessionMgr::FindClientByRuntimeID(runtimeID);
        if (client == nullptr)
        {
            continue;
        }

        int64_t socketID = client->getSocketID();  /*如果客户端刚好处于断线重连状态，则可能socketID 为-1*/
        if (socketID == -1)
        {
            continue;
        }

        if (IsPrintPacketSendedLog)
        {
            getService()->getService()->send(socketID, p, std::make_shared<DataSocket::PACKED_SENDED_CALLBACK::element_type>([socketID, realOP, realPacketLen]() {
                gDailyLogger->info("send complete to {}, op:{}, len:{}", socketID, realOP, realPacketLen);
            }));
        }
        else
        {
            getService()->getService()->send(socketID, p);
        }
    }
}

void LogicServerSession::onPrimaryServerIsSetClient(ReadPacket& rp)
{
    int64_t runtimeID = rp.readINT64();
    bool isSet = rp.readBool();
    auto client = ClientSessionMgr::FindClientByRuntimeID(runtimeID);
    if (client != nullptr)
    {
        client->setPrimaryServerID(isSet ? mID : -1);
    }
}

void LogicServerSession::onSlaveServerIsSetClient(ReadPacket& rp)
{
    int64_t runtimeID = rp.readINT64();
    bool isSet = rp.readBool();
    auto client = ClientSessionMgr::FindClientByRuntimeID(runtimeID);
    if (client != nullptr)
    {
        client->setSlaveServerID(isSet ? mID : -1);
    }
}

void LogicServerSession::onKickClientByRuntimeID(ReadPacket& rp)
{
    int64_t runtimeID = rp.readINT64();
    ClientSessionMgr::KickClientByRuntimeID(runtimeID);
}

void LogicServerSession::onPing(ReadPacket& rp)
{
    gDailyLogger->info("recv ping from logic server, id :{}", mID);

    TinyPacket sp(static_cast<PACKET_OP_TYPE>(CONNECTION_SERVER_SEND_OP::CONNECTION_SERVER_SEND_PONG));
    send(sp.getData(), sp.getLen());
}
