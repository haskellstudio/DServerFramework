#ifndef _USE_PACKET_SINGLE_EXENETSESSION_H
#define _USE_PACKET_SINGLE_EXENETSESSION_H

#include <memory>
#include "packet.h"
#include "NetSession.h"

/*  只重写onMsg--使用二进制packet协议作为通信的基础网络会话  */

class UsePacketSingleNetSession : public BaseNetSession, public std::enable_shared_from_this<UsePacketSingleNetSession>
{
public:
    UsePacketSingleNetSession() 
    {
    }

private:
    virtual size_t  onMsg(const char* buffer, size_t len) final;
    virtual void    procPacket(PACKET_OP_TYPE op, const char* body, PACKET_LEN_TYPE bodyLen) = 0;
};

#endif