#ifndef _LOGIC_CLIENT_MIRROR_H
#define _LOGIC_CLIENT_MIRROR_H

#include <stdint.h>
#include <unordered_map>
#include <memory>
#include <functional>

#include "packet.h"

class ReadPacket;
class Packet;
class ConnectionServerConnection;

/*  logic server上的客户端镜像 */
class ClientMirror : public std::enable_shared_from_this<ClientMirror>
{
public:
    typedef std::shared_ptr<ClientMirror>   PTR;
    typedef std::function<void(ClientMirror::PTR)> ENTER_HANDLE;
    typedef std::function<void(ClientMirror::PTR)> DISCONNECT_HANDLE;

    ClientMirror(int32_t csID, int64_t runtimeID);
    ~ClientMirror();

    typedef std::function<void(ClientMirror::PTR&, ReadPacket&)>   USER_MSG_HANDLE;

    /*重新设置客户端在连接服务器上的session id (可断线重连成功用)*/
    void            setSocketIDOnConnectionServer(int64_t socketID);

    int32_t         getConnectionServerID() const;
    int64_t         getRuntimeID() const;

    void            sendPacket(Packet& realPacket) const;
    void            sendPacket(const std::string& realPacketBinary) const;
    void            sendPacket(const char* buffer, size_t len) const;

    template<typename... Args>
    void            sendv(PACKET_OP_TYPE op, const Args&... args) const
    {
        BigPacket packet(op);
        packet.writev(args...);
        sendPacket(packet);
    }

    /*  请求客户端所在链接服务器设置(或取消设置)当前客户端的slave为当前logic server*/
    void            requestConnectionServerSlave(bool isSet) const;
public:
    static void     registerUserMsgHandle(PACKET_OP_TYPE, USER_MSG_HANDLE);

    static void     setClientEnterCallback(ENTER_HANDLE);
    static void     setClientDisConnectCallback(DISCONNECT_HANDLE);

    static  ENTER_HANDLE        getClientEnterCallback();
    static  DISCONNECT_HANDLE   getClientDisConnectCallback();
private:
    void            procData(const char* buffer, size_t len);
    void            sendToConnectionServer(Packet& packet) const;
private:
    const int32_t   mConnectionServerID;            /*  此客户端所属连接服务器的ID   */
    const int64_t   mRuntimeID;                     /*  此客户端在游戏运行时的ID    */
    int64_t         mSocketIDOnConnectionServer;    /*  此客户端在所属链接服务器上的socketid   */

private:
    static  std::unordered_map<PACKET_OP_TYPE, USER_MSG_HANDLE> sUserMsgHandlers;       /*  用户业务逻辑消息处理函数表   */
    static  ENTER_HANDLE                                        sEnterHandle;           /*  客户端的进入回调    */
    static  DISCONNECT_HANDLE                                   sDisConnectHandle;      /*  客户端的断开回调*/

    friend class ConnectionServerConnection;
};

class ClientMirrorMgr
{
public:
    typedef std::shared_ptr<ClientMirrorMgr> PTR;
    /*key 为客户端的RuntimeID*/
    typedef std::unordered_map<int64_t, ClientMirror::PTR> CLIENT_MIRROR_MAP;

public:
    ClientMirror::PTR                           FindClientByRuntimeID(int64_t id);
    void                                        AddClientOnRuntimeID(ClientMirror::PTR p, int64_t id);
    void                                        DelClientByRuntimeID(int64_t id);
    CLIENT_MIRROR_MAP&                          getAllClientMirror();

private:
    CLIENT_MIRROR_MAP                           mAllClientMirrorOnRuntimeID;
};

extern ClientMirrorMgr::PTR   gClientMirrorMgr;

#endif