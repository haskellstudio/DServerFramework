#include <unordered_map>
#include <set>
using namespace std;

#include "ConnectionServerSendOP.h"
#include "ConnectionServerRecvOP.h"
#include "ClientSession.h"
#include "WrapLog.h"
#include "packet.h"
#include "ClientLogicObject.h"
#include "ConnectionServerPassword.h"

#include "LogicServerSession.h"

extern int                                              gSelfID;
extern WrapLog::PTR gDailyLogger;
extern unordered_map<int, BaseNetSession::PTR>          gAllPrimaryServers;
extern unordered_map<int, BaseNetSession::PTR>          gAllSlaveServers;
extern WrapServer::PTR                                  gServer;

LogicServerSession::LogicServerSession()
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
            gAllPrimaryServers.erase(mID);
        }
        else
        {
            gAllSlaveServers.erase(mID);
        }
    }
}

bool LogicServerSession::checkPassword(const string& password)
{
    return password == ConnectionServerPassword::getInstance().getPassword();
}

void LogicServerSession::sendLogicServerLoginResult(bool isSuccess, const string& reason)
{
    TinyPacket sp(CONNECTION_SERVER_SEND_LOGICSERVER_RECVCSID);
    sp.writeINT32(gSelfID);
    sp.writeBool(isSuccess);
    sp.writeBinary(reason);
    sendPacket(sp.getData(), sp.getLen());
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
    int id  = rp.readINT32();
    bool isPrimary = rp.readBool();

    gDailyLogger->info("收到逻辑服务器登陆, ID:{},密码:{}", id, password);

    bool loginResult = false;
    string reason;

    if (checkPassword(password))
    {
        unordered_map<int, BaseNetSession::PTR>* servers = isPrimary ? &gAllPrimaryServers : &gAllSlaveServers;
        if (servers->find(id) == servers->end())
        {
            mIsPrimary = isPrimary;
            loginResult = true;
            mID = id;
            (*servers)[mID] = shared_from_this();
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
    if (rp.readBinary(realPacketBuff, realPacketLen))
    {
        int16_t destNum = rp.readINT16();

        DataSocket::PACKET_PTR p = DataSocket::makePacket(realPacketBuff, realPacketLen);

        ReadPacket realReadpacket(realPacketBuff, realPacketLen);
        PACKET_LEN_TYPE realPacketLen = realReadpacket.readPacketLen();
        PACKET_OP_TYPE realOP = realReadpacket.readOP();
        realReadpacket.skipAll();

        for (int i = 0; i < destNum; ++i)
        {
            int32_t csID = rp.readINT32();
            int64_t clientSocketID = rp.readINT64();

            if (csID == gSelfID)
            {
                if (IsPrintPacketSendedLog)
                {
                    gServer->getService()->send(clientSocketID, p, std::make_shared<DataSocket::PACKED_SENDED_CALLBACK::element_type>([clientSocketID, realOP, realPacketLen](){
                        gDailyLogger->info("send complete to {}, op:{}, len:{}", clientSocketID, realOP, realPacketLen);
                    }));
                }
                else
                {
                    gServer->getService()->send(clientSocketID, p);
                }
            }
        }
    }
}

void LogicServerSession::onPacket2ClientByRuntimeID(ReadPacket& rp)
{
    const char* realPacketBuff = nullptr;
    size_t realPacketLen = 0;
    if (rp.readBinary(realPacketBuff, realPacketLen))
    {
        int16_t destNum = rp.readINT16();

        ReadPacket realReadpacket(realPacketBuff, realPacketLen);
        PACKET_LEN_TYPE realPacketLen = realReadpacket.readPacketLen();
        PACKET_OP_TYPE realOP = realReadpacket.readOP();
        realReadpacket.skipAll();

        DataSocket::PACKET_PTR p = DataSocket::makePacket(realPacketBuff, realPacketLen);

        for (int i = 0; i < destNum; ++i)
        {
            int64_t runtimeID = rp.readINT64();
            ClientObject::PTR client = findClientByRuntimeID(runtimeID);
            if (client != nullptr)
            {
                int64_t socketID = client->getSocketID();  /*如果客户端刚好处于断线重连状态，则可能socketID 为-1*/
                if (socketID != -1)
                {
                    if (IsPrintPacketSendedLog)
                    {
                        gServer->getService()->send(socketID, p, std::make_shared<DataSocket::PACKED_SENDED_CALLBACK::element_type>([socketID, realOP, realPacketLen](){
                            gDailyLogger->info("send complete to {}, op:{}, len:{}", socketID, realOP, realPacketLen);
                        }));
                    }
                    else
                    {
                        gServer->getService()->send(socketID, p);
                    }
                }
            }
        }
    }
}

void LogicServerSession::onSlaveServerIsSetClient(ReadPacket& rp)
{
    int64_t runtimeID = rp.readINT64();
    bool isSet = rp.readBool();
    ClientObject::PTR p = findClientByRuntimeID(runtimeID);
    if (p != nullptr)
    {
        p->setSlaveServerID(isSet ? mID : -1);
    }
}

void LogicServerSession::onKickClientByRuntimeID(ReadPacket& rp)
{
    int64_t runtimeID = rp.readINT64();
    ClientObject::PTR p = findClientByRuntimeID(runtimeID);
    if (p != nullptr)
    {
        int64_t socketID = p->getSocketID();
        if (socketID != -1)
        {
            gDailyLogger->warn("kick client, runtime id: {}, socket id{}", runtimeID, socketID);
            gServer->getService()->disConnect(socketID);
        }
    }
}

void LogicServerSession::onPing(ReadPacket& rp)
{
    gDailyLogger->info("recv ping from logic server, id :{}", mID);

    TinyPacket sp(CONNECTION_SERVER_SEND_PONG);
    sendPacket(sp.getData(), sp.getLen());
}
