#include <iostream>
using namespace std;

#include "packet.h"
#include "ConnectionServerRecvOP.h"
#include "ConnectionServerSendOP.h"
#include "CenterServerRecvOP.h"

#include "CenterServerConnection.h"
#include "ClientMirror.h"
#include "timer.h"
#include "WrapLog.h"

#include "UsePacketExtNetSession.h"
#include "../../ServerConfig/ServerConfig.pb.h"
#include "ConnectionServerConnection.h"

extern ServerConfig::LogicServerConfig logicServerConfig;
extern ClientMirrorMgr::PTR   gClientMirrorMgr;
extern WrapServer::PTR   gServer;
extern WrapLog::PTR gDailyLogger;
extern TimerMgr::PTR    gTimerMgr;
unordered_map<int32_t, ConnectionServerConnection::PTR>     gAllLogicConnectionServerClient;

unordered_map<int32_t, std::tuple<string, int>> alreadyConnectingServers;
std::mutex alreadyConnectingServersLock;

bool isAlreadyConnecting(int32_t id)
{
    bool ret = false;
    alreadyConnectingServersLock.lock();
    ret = alreadyConnectingServers.find(id) != alreadyConnectingServers.end();
    alreadyConnectingServersLock.unlock();
    return ret;
}

void eraseConnectingServer(int32_t id)
{
    alreadyConnectingServersLock.lock();
    alreadyConnectingServers.erase(id);
    alreadyConnectingServersLock.unlock();
}

void addConnectingServer(int32_t id, string& ip, int port)
{
    alreadyConnectingServersLock.lock();
    alreadyConnectingServers[id] = std::make_tuple(ip, port);
    alreadyConnectingServersLock.unlock();
}

void tryCompareConnect(unordered_map<int32_t, std::tuple<string, int, string>>& servers)
{
    alreadyConnectingServersLock.lock();
    for (auto& v : servers)
    {
        if (alreadyConnectingServers.find(v.first) == alreadyConnectingServers.end())
        {
            int idInEtcd = v.first;
            string ip = std::get<0>(v.second);
            int port = std::get<1>(v.second);
            string password = std::get<2>(v.second);

            thread([ip, port, idInEtcd, password](){
                gDailyLogger->info("ready connect connection server id:{}, addr :{}:{}", idInEtcd, ip, port);
                sock fd = ox_socket_connect(false, ip.c_str(), port);
                if (fd != SOCKET_ERROR)
                {
                    gDailyLogger->info("connect success {}:{}", ip, port);
                    WrapAddNetSession(gServer, fd, make_shared<UsePacketExtNetSession>(std::make_shared<ConnectionServerConnection>(idInEtcd, port, password)), 10000, 32 * 1024 * 1024);
                }
                else
                {
                    gDailyLogger->error("connect failed {}:{}", ip, port);
                }
            }).detach();
        }
    }
    alreadyConnectingServersLock.unlock();
}

ConnectionServerConnection::ConnectionServerConnection(int idInEtcd, int port, std::string password) : BaseLogicSession()
{
    mIsSuccess = false;
    mPort = port;
    mIDInEtcd = idInEtcd;
    mConnectionServerID = -1;
    mPassword = password;
}

ConnectionServerConnection::~ConnectionServerConnection()
{
    if (mPingTimer.lock())
    {
        mPingTimer.lock()->Cancel();
    }
}

void ConnectionServerConnection::onEnter()
{
    if (!isAlreadyConnecting(mIDInEtcd))
    {
        mIsSuccess = true;
        addConnectingServer(mIDInEtcd, getIP(), mPort);

        gDailyLogger->warn("connect connection server success ");
        /*  发送自己的ID给连接服务器 */
        TinyPacket packet(CONNECTION_SERVER_RECV_LOGICSERVER_LOGIN);
        packet.writeBinary(mPassword);
        packet.writeINT32(logicServerConfig.id());
        packet.writeBool(logicServerConfig.isprimary());

        sendPacket(packet);

        ping();
    }
}

void ConnectionServerConnection::ping()
{
    TinyPacket p(CONNECTION_SERVER_RECV_PING);

    sendPacket(p);

    mPingTimer = gTimerMgr->AddTimer(5000, [this](){
        ping();
    });
}

void ConnectionServerConnection::onClose()
{
    gDailyLogger->warn("dis connect of connection server");
    gAllLogicConnectionServerClient.erase(mConnectionServerID);

    if (mIsSuccess)
    {
        eraseConnectingServer(mIDInEtcd);
    }
}

void ConnectionServerConnection::onMsg(const char* data, size_t len)
{
    ReadPacket rp(data, len);
    rp.readPacketLen();
    PACKET_OP_TYPE op = rp.readOP();

    switch (op)
    {
        case CONNECTION_SERVER_SEND_PONG:
        {

        }
        break;
        case CONNECTION_SERVER_SEND_LOGICSERVER_RECVCSID:
        {
            /*收到所链接的连接服务器的ID*/
            mConnectionServerID = rp.readINT32();
            bool isSuccess = rp.readBool();
            string reason = rp.readBinary();

            if (isSuccess)
            {
                gDailyLogger->info("登陆链接服务器 {} 成功", mConnectionServerID);
                assert(gAllLogicConnectionServerClient.find(mConnectionServerID) == gAllLogicConnectionServerClient.end());
                gAllLogicConnectionServerClient[mConnectionServerID] = shared_from_this();
            }
            else
            {
                gDailyLogger->error("登陆链接服务器 {} 失败,原因:{}", mConnectionServerID, reason);
            }
        }
        break;
        case CONNECTION_SERVER_SEND_LOGICSERVER_INIT_CLIENTMIRROR:
        {
            int64_t socketID = rp.readINT64();
            int64_t runtimeID = rp.readINT64();

            gDailyLogger->info("recv init client {} :{}", socketID, runtimeID);

            ClientMirror::PTR p = std::make_shared<ClientMirror>(runtimeID, mConnectionServerID, socketID);
            gClientMirrorMgr->AddClientOnRuntimeID(p, runtimeID);

            auto callback = ClientMirror::getClientEnterCallback();
            if (callback != nullptr)
            {
                callback(p);
            }
        }
        break;
        case CONNECTION_SERVER_SEND_LOGICSERVER_DESTROY_CLIENT:
        {
            int64_t runtimeID = rp.readINT64();
            gDailyLogger->info("recv destroy client, runtime id:{}", runtimeID);
            ClientMirror::PTR client = gClientMirrorMgr->FindClientByRuntimeID(runtimeID);
            gClientMirrorMgr->DelClientByRuntimeID(runtimeID);
            auto callback = ClientMirror::getClientDisConnectCallback();
            if (callback != nullptr)
            {
                callback(client);
            }
        }
        break;
        case CONNECTION_SERVER_SEND_LOGICSERVER_FROMCLIENT:
        {
            /*  表示收到链接服转发过来的客户端消息包   */
            int64_t clientRuntimeID = rp.readINT64();
            const char* s = nullptr;
            size_t len = 0;
            rp.readBinary(s, len);
            if (s != nullptr)
            {
                ClientMirror::PTR client = gClientMirrorMgr->FindClientByRuntimeID(clientRuntimeID);
                if (client != nullptr)
                {
                    client->procData(s, len);
                }
            }
        }
        break;
        default:
        {
            assert(false);
        }
        break;
    }
}

void ConnectionServerConnection::sendPacket(Packet& packet)
{
    send(packet.getData(), packet.getLen());
}