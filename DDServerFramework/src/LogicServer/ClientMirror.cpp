#include <algorithm>

using namespace std;

#include "packet.h"
#include "ConnectionServerConnection.h"
#include "CenterServerConnection.h"
#include "ConnectionServerRecvOP.h"
#include "CenterServerRecvOP.h"

#include "ClientMirror.h"

std::unordered_map<PACKET_OP_TYPE, ClientMirror::USER_MSG_HANDLE> ClientMirror::sUserMsgHandlers;
ClientMirrorMgr::PTR  gClientMirrorMgr;
ClientMirror::ENTER_HANDLE ClientMirror::sEnterHandle = nullptr;
ClientMirror::DISCONNECT_HANDLE ClientMirror::sDisConnectHandle = nullptr;

ClientMirror::PTR ClientMirrorMgr::FindClientByRuntimeID(int64_t id)
{
    auto it = mAllClientMirrorOnRuntimeID.find(id);
    if (it != mAllClientMirrorOnRuntimeID.end())
    {
        return (*it).second;
    }
    else
    {
        return nullptr;
    }
}

void ClientMirrorMgr::AddClientOnRuntimeID(ClientMirror::PTR p, int64_t id)
{
    mAllClientMirrorOnRuntimeID[id] = p;
}

void ClientMirrorMgr::DelClientByRuntimeID(int64_t id)
{
    mAllClientMirrorOnRuntimeID.erase(id);
}

ClientMirrorMgr::CLIENT_MIRROR_MAP& ClientMirrorMgr::getAllClientMirror()
{
    return mAllClientMirrorOnRuntimeID;
}

ClientMirror::ClientMirror(int64_t runtimeID, int csID, int64_t socketID) : mRuntimeID(runtimeID), mConnectionServerID(csID), mSocketIDOnConnectionServer(socketID)
{
    mConnectionServer = gAllLogicConnectionServerClient[mConnectionServerID];
}

ClientMirror::~ClientMirror()
{
}

void ClientMirror::sendPacket(Packet& realPacket) const
{
    sendPacket(realPacket.getData(), realPacket.getLen());
}

void ClientMirror::sendPacket(const std::string& realPacketBinary) const
{
    /*构造消息发送到网络线程*/
    BigPacket sp(CONNECTION_SERVER_RECV_PACKET2CLIENT_BYSOCKINFO);
    sp.writeBinary(realPacketBinary);
    sp.writeINT16(1);
    sp.writeINT32(mConnectionServerID);
    sp.writeINT64(mSocketIDOnConnectionServer);

    sendToConnectionServer(sp);
}

void ClientMirror::sendPacket(const char* buffer, size_t len) const
{
    /*构造消息发送到网络线程*/
    BigPacket sp(CONNECTION_SERVER_RECV_PACKET2CLIENT_BYSOCKINFO);
    sp.writeBinary(buffer, len);
    sp.writeINT16(1);
    sp.writeINT32(mConnectionServerID);
    sp.writeINT64(mSocketIDOnConnectionServer);

    sendToConnectionServer(sp);
}

int64_t ClientMirror::getRuntimeID() const
{
    return mRuntimeID;
}

int32_t ClientMirror::getConnectionServerID() const
{
    return mConnectionServerID;
}

void ClientMirror::requestConnectionServerSlave(bool isSet) const
{
    TinyPacket sp(CONNECTION_SERVER_RECV_IS_SETCLIENT_SLAVEID);
    sp.writeINT64(mRuntimeID);
    sp.writeBool(isSet);

    sendToConnectionServer(sp);
}

void ClientMirror::registerUserMsgHandle(PACKET_OP_TYPE op, USER_MSG_HANDLE handle)
{
    assert(sUserMsgHandlers.find(op) == sUserMsgHandlers.end());
    sUserMsgHandlers[op] = handle;
}

void ClientMirror::setClientEnterCallback(ClientMirror::ENTER_HANDLE handle)
{
    sEnterHandle = handle;
}

void ClientMirror::setClientDisConnectCallback(DISCONNECT_HANDLE handle)
{
    sDisConnectHandle = handle;
}

ClientMirror::ENTER_HANDLE ClientMirror::getClientEnterCallback()
{
    return sEnterHandle;
}

ClientMirror::DISCONNECT_HANDLE ClientMirror::getClientDisConnectCallback()
{
    return sDisConnectHandle;
}

void ClientMirror::procData(const char* buffer, size_t len)
{
    ReadPacket rp(buffer, len);
    PACKET_LEN_TYPE packetlen = rp.readPacketLen();
    assert(packetlen == len);
    PACKET_OP_TYPE op = rp.readOP();
    auto handleIT = sUserMsgHandlers.find(op);
    if (handleIT != sUserMsgHandlers.end())
    {
        (*handleIT).second(shared_from_this(), rp);
    }
}

void ClientMirror::sendToConnectionServer(Packet& packet) const
{
    auto cs = mConnectionServer.lock();
    if (cs != nullptr)
    {
        cs->sendPacket(packet);
    }
}