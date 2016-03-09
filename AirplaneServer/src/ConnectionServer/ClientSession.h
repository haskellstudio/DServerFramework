#ifndef _CLIENT_SESSION_H
#define _CLIENT_SESSION_H

#include "NetSession.h"
#include "ClientLogicObject.h"
#include "UsePacketSingleNetSession.h"

/*  玩家链接会话(玩家网络掉线，此对象就销毁)    */
class ConnectionClientSession : public UsePacketSingleNetSession
{
public:
    typedef shared_ptr<ConnectionClientSession> PTR;

    ConnectionClientSession();
    ~ConnectionClientSession();

    ClientObject::PTR   getClientObject();

private:
    virtual void        onEnter() override;
    virtual void        onClose() override;
    virtual void        procPacket(PACKET_OP_TYPE op, const char* body, PACKET_LEN_TYPE bodyLen);

private:
    /*  客户端请求断线重连(新启动的一个链接，请求重连之前的某一个ClientObject    */
    void                reConnect(const char* packerBuffer, PACKET_OP_TYPE packetLen);
private:
    ClientObject::PTR   mClient;    /*逻辑层的客户端对象*/
    int                 mPingCount;
};
#endif