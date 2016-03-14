#include <algorithm>

using namespace std;

#include "MakeUID.h"
#include "packet.h"
#include "ConnectionServerRecvOP.h"
#include "ConnectionServerConnection.h"
#include "CenterServerConnection.h"
#include "CenterServerRecvOP.h"
#include "WrapJsonValue.h"
#include "WrapLog.h"
#include "MakeUID.h"
#include "HelpFunction.h"
#include "google/protobuf/text_format.h"

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "luaconf.h"
};
#include "lua_tinker.h"

#include "ClientMirror.h"

extern struct lua_State* gLua;
extern WrapLog::PTR  gDailyLogger;
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

ClientMirror::ClientMirror(int32_t csID, int64_t runtimeID) : mConnectionServerID(csID), mRuntimeID(runtimeID)
{
    mSocketIDOnConnectionServer = -1;
}

ClientMirror::~ClientMirror()
{
}


void ClientMirror::sendPacket(Packet& packet)
{
    /*构造消息发送到网络线程*/
    BigPacket sp(CONNECTION_SERVER_RECV_PACKET2CLIENT_BYSOCKINFO);
    sp.writeBinary(packet.getData(), packet.getLen());
    sp.writeINT16(1);
    sp.writeINT32(mConnectionServerID);
    sp.writeINT64(mSocketIDOnConnectionServer);

    ConnectionServerConnection* l = gAllLogicConnectionServerClient[mConnectionServerID];
    if (l != nullptr)
    {
        l->sendPacket(sp);
    }
}

void ClientMirror::sendPacket(const std::string& realPacketBinary)
{
    /*构造消息发送到网络线程*/
    BigPacket sp(CONNECTION_SERVER_RECV_PACKET2CLIENT_BYSOCKINFO);
    sp.writeBinary(realPacketBinary);
    sp.writeINT16(1);
    sp.writeINT32(mConnectionServerID);
    sp.writeINT64(mSocketIDOnConnectionServer);

    ConnectionServerConnection* l = gAllLogicConnectionServerClient[mConnectionServerID];
    if (l != nullptr)
    {
        l->sendPacket(sp);
    }
}

void ClientMirror::sendPacket(const char* buffer, size_t len)
{
    /*构造消息发送到网络线程*/
    BigPacket sp(CONNECTION_SERVER_RECV_PACKET2CLIENT_BYSOCKINFO);
    sp.writeBinary(buffer, len);
    sp.writeINT16(1);
    sp.writeINT32(mConnectionServerID);
    sp.writeINT64(mSocketIDOnConnectionServer);

    ConnectionServerConnection* l = gAllLogicConnectionServerClient[mConnectionServerID];
    if (l != nullptr)
    {
        l->sendPacket(sp);
    }
}

void ClientMirror::setSocketIDOnConnectionServer(int64_t socketID)
{
    mSocketIDOnConnectionServer = socketID;
}

int64_t ClientMirror::getRuntimeID() const
{
    return mRuntimeID;
}

int32_t ClientMirror::getConnectionServerID() const
{
    return mConnectionServerID;
}

void ClientMirror::requestConnectionServerSlave(bool isSet)
{
    TinyPacket sp(CONNECTION_SERVER_RECV_IS_SETCLIENT_SLAVEID);
    sp.writeINT64(mRuntimeID);
    sp.writeBool(isSet);

    ConnectionServerConnection* cs = gAllLogicConnectionServerClient[mConnectionServerID];
    if (cs != nullptr)
    {
        cs->sendPacket(sp);
    }
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
        (*handleIT).second(*this, rp);
    }
}
