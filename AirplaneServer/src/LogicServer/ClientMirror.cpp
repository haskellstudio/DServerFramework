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

ClientMirror* ClientMirrorMgr::FindClientByRuntimeID(int64_t id)
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

void ClientMirrorMgr::AddClientOnRuntimeID(ClientMirror* p, int64_t id)
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

ClientMirror::ClientMirror()
{
    mConnectionServerID = -1;
    mSocketIDOnConnectionServer = -1;
    mRuntimeID = -1;
}

ClientMirror::~ClientMirror()
{
}


void ClientMirror::sendPacket(Packet& packet)
{
    /*构造消息发送到网络线程*/
    BigPacket sp;
    sp.setOP(CONNECTION_SERVER_RECV_PACKET2CLIENT_BYSOCKINFO);
    sp.writeBinary(packet.getData(), packet.getLen());
    sp.writeINT16(1);
    sp.writeINT32(mConnectionServerID);
    sp.writeINT64(mSocketIDOnConnectionServer);
    sp.end();

    ConnectionServerConnection* l = gAllLogicConnectionServerClient[mConnectionServerID];
    if (l != nullptr)
    {
        l->sendPacket(sp);
    }
}

void ClientMirror::sendPacket(const std::string& realPacketBinary)
{
    /*构造消息发送到网络线程*/
    BigPacket sp;
    sp.setOP(CONNECTION_SERVER_RECV_PACKET2CLIENT_BYSOCKINFO);
    sp.writeBinary(realPacketBinary);
    sp.writeINT16(1);
    sp.writeINT32(mConnectionServerID);
    sp.writeINT64(mSocketIDOnConnectionServer);
    sp.end();

    ConnectionServerConnection* l = gAllLogicConnectionServerClient[mConnectionServerID];
    if (l != nullptr)
    {
        l->sendPacket(sp);
    }
}

void ClientMirror::sendPacket(const char* buffer, size_t len)
{
    /*构造消息发送到网络线程*/
    BigPacket sp;
    sp.setOP(CONNECTION_SERVER_RECV_PACKET2CLIENT_BYSOCKINFO);
    sp.writeBinary(buffer, len);
    sp.writeINT16(1);
    sp.writeINT32(mConnectionServerID);
    sp.writeINT64(mSocketIDOnConnectionServer);
    sp.end();

    ConnectionServerConnection* l = gAllLogicConnectionServerClient[mConnectionServerID];
    if (l != nullptr)
    {
        l->sendPacket(sp);
    }
}

void ClientMirror::setRuntimeInfo(int32_t csID, int64_t socketID, int64_t runtimeID)
{
    mConnectionServerID = csID;
    mSocketIDOnConnectionServer = socketID;
    mRuntimeID = runtimeID;
}

int64_t ClientMirror::getRuntimeID() const
{
    return mRuntimeID;
}

void ClientMirror::resetSocketInfo(int64_t socketID)
{
    mSocketIDOnConnectionServer = socketID;
}

int32_t ClientMirror::getConnectionServerID() const
{
    return mConnectionServerID;
}

void ClientMirror::requestConnectionServerSlave(bool isSet)
{
    FixedPacket<128> sp;
    sp.setOP(CONNECTION_SERVER_RECV_IS_SETCLIENT_SLAVEID);
    sp.writeINT64(mRuntimeID);
    sp.writeBool(isSet);
    sp.end();

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
