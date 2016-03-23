#ifndef _CLIENT_LOGIC_OBJECT_H
#define _CLIENT_LOGIC_OBJECT_H

#include <unordered_map>
#include <stdint.h>
#include <memory>

#include "packet.h"
#include "timer.h"
#include "NetSession.h"

class Packet;

/*  链接服务器上的客户端对象(用于将网络对象分离-支持断线重连)(此对象存在则表示客户端还处于服务器，逻辑并未离线) */
class ClientObject : public std::enable_shared_from_this<ClientObject>
{
public:
    typedef std::shared_ptr<ClientObject> PTR;
    typedef std::weak_ptr<ClientObject> WEAK_PTR;

    explicit ClientObject(int64_t id);

    void                resetSocketID(int64_t id);
    int64_t             getSocketID() const;

    ~ClientObject();

    /*通知内部服务器，此客户端网络断开*/
    void                notifyDisConnect();

    int64_t             getRuntimeID() const;

    void                setClosed();

    void                procPacket(PACKET_OP_TYPE op, const char* packerBuffer, PACKET_OP_TYPE packetLen);

    void                setSlaveServerID(int id);
    int                 getPrimaryServerID() const;

private:
    /*TODO::是否保持所在的服务器会话指针，避免每次查找*/
    void                sendPacketToPrimaryServer(Packet& packet);

    void                sendPacketToSlaveServer(Packet& packet);

    void                _sendPacketToServer(Packet& packet, std::unordered_map<int32_t, BaseNetSession::PTR>& servers, int32_t serverID);

    void                claimRuntimeID();
    void                claimPrimaryServer();

private:
    int64_t             mSocketID;
    int64_t             mRuntimeID;

    /*客户端链接网关后，会自动为其分配primary 型别的logic server*/
    int                 mPrimaryServerID;       /*  自动分配给客户端的logic server id    */

    /*由内部logic server进行设置客户端的slave server，当其存在后，客户端发送的任意消息包都转发给它*/
    int                 mSlaveServerID;         /*  临时接管客户端的logic server id */
};

ClientObject::PTR findClientByRuntimeID(int64_t runtimeID);
void    addClientByRuntimeID(ClientObject::PTR client, int64_t runtimeID);
void    eraseClientByRuntimeID(int64_t runtimeID);
void    kickClientByRuntimeID(int64_t runtimeID);
void    kickClientOfPrimary(int primaryServerID);

#endif