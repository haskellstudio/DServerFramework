#ifndef _CLIENT_SESSION_H
#define _CLIENT_SESSION_H

#include "NetSession.h"
#include "ClientLogicObject.h"
#include "UsePacketSingleNetSession.h"

/*  客户端链接会话(客户端网络掉线，此对象就销毁)    */
class ConnectionClientSession : public UsePacketSingleNetSession
{
public:
    typedef std::shared_ptr<ConnectionClientSession> PTR;

    ConnectionClientSession();
    ~ConnectionClientSession();

    ClientObject::PTR&  getClientObject();

private:
    virtual void        onEnter() override;
    virtual void        onClose() override;
    virtual void        procPacket(PACKET_OP_TYPE op, const char* body, PACKET_LEN_TYPE bodyLen);

private:
    ClientObject::PTR   mClient;    /*逻辑层的客户端对象*/
};
#endif