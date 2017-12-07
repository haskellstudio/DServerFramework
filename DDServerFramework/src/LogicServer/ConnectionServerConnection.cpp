#include <iostream>
using namespace std;

#include <brynet/utils/packet.h> 
#include <brynet/timer/timer.h>
#include "ConnectionServerRecvOP.h"
#include "ConnectionServerSendOP.h"
#include "CenterServerRecvOP.h"

#include "CenterServerConnection.h"
#include "ClientMirror.h"
#include "WrapLog.h"

#include "UsePacketExtNetSession.h"
#include "../../ServerConfig/ServerConfig.pb.h"
#include "GlobalValue.h"
#include "ConnectionServerConnection.h"

static unordered_map<int32_t, ConnectionServerConnection::PTR>     gAllLogicConnectionServerClient;
static unordered_map<int32_t, std::tuple<string, int>> alreadyConnectingServers;
static std::mutex alreadyConnectingServersLock;

static bool isAlreadyConnecting(int32_t id)
{
    std::lock_guard<std::mutex> lck(alreadyConnectingServersLock);
    bool ret = false;
    ret = alreadyConnectingServers.find(id) != alreadyConnectingServers.end();
    return ret;
}

static void eraseConnectingServer(int32_t id)
{
    std::lock_guard<std::mutex> lck(alreadyConnectingServersLock);
    alreadyConnectingServers.erase(id);
}

static void addConnectingServer(int32_t id, string& ip, int port)
{
    std::lock_guard<std::mutex> lck(alreadyConnectingServersLock);
    alreadyConnectingServers[id] = std::make_tuple(ip, port);
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
        mPingTimer.lock()->cancel();
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

    mPingTimer = gLogicTimerMgr->addTimer(std::chrono::microseconds(5000), [shared_this = std::static_pointer_cast<ConnectionServerConnection>(shared_from_this())](){
        shared_this->ping();
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

void ConnectionServerConnection::initClient(int64_t runtimeID, int64_t socketID)
{
    gDailyLogger->info("init client {} :{}", socketID, runtimeID);

    if (gClientMirrorMgr->FindClientByRuntimeID(runtimeID) != nullptr)
    {
        return;
    }

    ClientMirror::PTR p = std::make_shared<ClientMirror>(runtimeID, mConnectionServerID, socketID);
    gClientMirrorMgr->AddClientOnRuntimeID(p, runtimeID);

    auto callback = gClientMirrorMgr->getClientEnterCallback();
    if (callback != nullptr)
    {
        callback(p);
    }
}

void ConnectionServerConnection::onMsg(const char* data, size_t len)
{
    ReadPacket rp(data, len);
    rp.readPacketLen();
    CONNECTION_SERVER_SEND_OP op = static_cast<CONNECTION_SERVER_SEND_OP>(rp.readOP());

    switch (op)
    {
        case CONNECTION_SERVER_SEND_OP::CONNECTION_SERVER_SEND_PONG:
        {

        }
        break;
        case CONNECTION_SERVER_SEND_OP::CONNECTION_SERVER_SEND_LOGICSERVER_RECVCSID:
        {
            /*收到所链接的连接服务器的ID*/
            mConnectionServerID = rp.readINT32();
            bool isSuccess = rp.readBool();
            string reason = rp.readBinary();

            if (isSuccess)
            {
                gDailyLogger->info("登陆链接服务器 {} 成功", mConnectionServerID);
                assert(gAllLogicConnectionServerClient.find(mConnectionServerID) == gAllLogicConnectionServerClient.end());
                gAllLogicConnectionServerClient[mConnectionServerID] = std::static_pointer_cast<ConnectionServerConnection>(shared_from_this());
            }
            else
            {
                gDailyLogger->error("登陆链接服务器 {} 失败,原因:{}", mConnectionServerID, reason);
            }
        }
        break;
        case CONNECTION_SERVER_SEND_OP::CONNECTION_SERVER_SEND_LOGICSERVER_INIT_CLIENTMIRROR:
        {
            int64_t socketID = rp.readINT64();
            int64_t runtimeID = rp.readINT64();

            initClient(runtimeID, socketID);
        }
        break;
        case CONNECTION_SERVER_SEND_OP::CONNECTION_SERVER_SEND_LOGICSERVER_DESTROY_CLIENT:
        {
            int64_t runtimeID = rp.readINT64();
            gDailyLogger->info("recv destroy client, runtime id:{}", runtimeID);
            ClientMirror::PTR client = gClientMirrorMgr->FindClientByRuntimeID(runtimeID);
            gClientMirrorMgr->DelClientByRuntimeID(runtimeID);
            auto callback = gClientMirrorMgr->getClientDisConnectCallback();
            if (callback != nullptr)
            {
                callback(client);
            }
        }
        break;
        case CONNECTION_SERVER_SEND_OP::CONNECTION_SERVER_SEND_LOGICSERVER_FROMCLIENT:
        {
            /*  表示收到链接服转发过来的客户端消息包   */
            int64_t clientSocketID = rp.readINT64();
            int64_t clientRuntimeID = rp.readINT64();
            const char* s = nullptr;
            size_t len = 0;
            rp.readBinary(s, len);
            if (s != nullptr)
            {
                ClientMirror::PTR client = gClientMirrorMgr->FindClientByRuntimeID(clientRuntimeID);
                if (client == nullptr)
                {
                    initClient(clientRuntimeID, clientSocketID);
                    client = gClientMirrorMgr->FindClientByRuntimeID(clientRuntimeID);
                }

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

ConnectionServerConnection::PTR ConnectionServerConnection::FindConnectionServerByID(int32_t id)
{
    auto it = gAllLogicConnectionServerClient.find(id);
    if (it != gAllLogicConnectionServerClient.end())
    {
        return (*it).second;
    }
    else
    {
        return nullptr;
    }
}

void tryCompareConnect(unordered_map<int32_t, std::tuple<string, int, string>>& servers)
{
    std::lock_guard<std::mutex> lck(alreadyConnectingServersLock);
    for (auto& v : servers)
    {
        if (alreadyConnectingServers.find(v.first) != alreadyConnectingServers.end())
        {
            continue;
        }

        int idInEtcd = v.first;
        string ip = std::get<0>(v.second);
        int port = std::get<1>(v.second);
        string password = std::get<2>(v.second);

        thread([ip, port, idInEtcd, password]() {
            gDailyLogger->info("ready connect connection server id:{}, addr :{}:{}", idInEtcd, ip, port);
            sock fd = ox_socket_connect(false, ip.c_str(), port);
            if (fd != SOCKET_ERROR)
            {
                gDailyLogger->info("connect success {}:{}", ip, port);
                WrapAddNetSession(gServer, fd, make_shared<UsePacketExtNetSession>(std::make_shared<ConnectionServerConnection>(idInEtcd, port, password)), 
                    std::chrono::microseconds(10000),
                    32 * 1024 * 1024);
            }
            else
            {
                gDailyLogger->error("connect failed {}:{}", ip, port);
            }
        }).detach();
    }
}