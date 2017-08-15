#include <algorithm>

#include "packet.h"
#include "ConnectionServerConnection.h"
#include "CenterServerConnection.h"
#include "ConnectionServerRecvOP.h"
#include "CenterServerRecvOP.h"
#include "GlobalValue.h"
#include "ClientMirror.h"

ClientMirror::ClientMirror(int64_t runtimeID, int csID, int64_t socketID) :
    mRuntimeID(runtimeID), mConnectionServerID(csID), mSocketIDOnConnectionServer(socketID)
{
    mConnectionServer = ConnectionServerConnection::FindConnectionServerByID(mConnectionServerID);
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

void ClientMirror::procData(const char* buffer, size_t len)
{
    ReadPacket rp(buffer, len);
    PACKET_LEN_TYPE packetlen = rp.readPacketLen();
    assert(packetlen == len);
    PACKET_OP_TYPE op = rp.readOP();
    auto handle = gClientMirrorMgr->findUserMsgHandle(op);
    if (handle != nullptr)
    {
        handle(shared_from_this(), rp);
    }
    rp.skipAll();
}

void ClientMirror::sendToConnectionServer(Packet& packet) const
{
    auto cs = mConnectionServer.lock();
    if (cs != nullptr)
    {
        cs->sendPacket(packet);
    }
}

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

void ClientMirrorMgr::registerUserMsgHandle(PACKET_OP_TYPE op, USER_MSG_HANDLE handle)
{
    assert(mUserMsgHandlers.find(op) == mUserMsgHandlers.end());
    mUserMsgHandlers[op] = handle;
}

ClientMirrorMgr::USER_MSG_HANDLE ClientMirrorMgr::findUserMsgHandle(PACKET_OP_TYPE op)
{
    auto handleIT = mUserMsgHandlers.find(op);
    if (handleIT != mUserMsgHandlers.end())
    {
        return (*handleIT).second;
    }

    return nullptr;
}

void ClientMirrorMgr::setClientEnterCallback(ClientMirrorMgr::ENTER_HANDLE handle)
{
    mEnterHandle = handle;
}

void ClientMirrorMgr::setClientDisConnectCallback(DISCONNECT_HANDLE handle)
{
    mDisConnectHandle = handle;
}

ClientMirrorMgr::ENTER_HANDLE ClientMirrorMgr::getClientEnterCallback()
{
    return mEnterHandle;
}

ClientMirrorMgr::DISCONNECT_HANDLE ClientMirrorMgr::getClientDisConnectCallback()
{
    return mDisConnectHandle;
}