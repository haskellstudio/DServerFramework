#ifndef _LOGIC_CLIENT_MIRROR_H
#define _LOGIC_CLIENT_MIRROR_H

#include <stdint.h>
#include <unordered_map>
#include <memory>
#include <functional>

#include "packet.h"

class ReadPacket;
class Packet;
class WrapJsonValue;
class ConnectionServerConnection;

/*  logic server上的客户端镜像 */
class ClientMirror
{
public:
    ClientMirror();
    ~ClientMirror();

    typedef std::function<void(ClientMirror&, ReadPacket&)>   USER_MSG_HANDLE;

    int32_t         getConnectionServerID() const;

    void            setRuntimeInfo(int32_t csID, int64_t socketID, int64_t runtimeID);
    int64_t         getRuntimeID() const;

    void            sendPacket(Packet& packet);
    void            sendPacket(const std::string& realPacketBinary);
    void            sendPacket(const char* buffer, size_t len);

    /*重新设置玩家在连接服务器上的session id (断线重连成功用)*/
    void            resetSocketInfo(int64_t socketID);

    template<typename... Args>
    void            sendv(int16_t op, const Args&... args)
    {
        BigPacket packet(op);
        packet.writev(args...);
        sendPacket(packet);
    }

    void            requestConnectionServerSlave(bool isSet);
public:
    static void     registerUserMsgHandle(PACKET_OP_TYPE, USER_MSG_HANDLE);

private:
    void            procData(const char* buffer, size_t len);

private:
    int32_t         mConnectionServerID;            /*  此玩家所属连接服务器的ID   */
    int64_t         mSocketIDOnConnectionServer;    /*  此玩家在所属链接服务器上的socketid   */
    int64_t         mRuntimeID;                     /*  此玩家在游戏运行时的ID    */

private:
    static  std::unordered_map<PACKET_OP_TYPE, USER_MSG_HANDLE> sUserMsgHandlers;

    friend class ConnectionServerConnection;
};


class ClientMirrorMgr
{
public:
    typedef std::shared_ptr<ClientMirrorMgr> PTR;
    /*key 为客户端的RuntimeID*/
    typedef std::unordered_map<int64_t, ClientMirror*> CLIENT_MIRROR_MAP;
public:
    ClientMirror*                               FindClientByRuntimeID(int64_t id);
    void                                        AddClientOnRuntimeID(ClientMirror* p, int64_t id);
    void                                        DelClientByRuntimeID(int64_t id);


    CLIENT_MIRROR_MAP&                          getAllClientMirror();
private:
    CLIENT_MIRROR_MAP                           mAllClientMirrorOnRuntimeID;
};

extern ClientMirrorMgr::PTR   gClientMirrorMgr;

#endif