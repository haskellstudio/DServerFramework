#ifndef _USE_WEB_PACKET_SINGLE_EXENETSESSION_H
#define _USE_WEB_PACKET_SINGLE_EXENETSESSION_H

#include <memory>
#include "packet.h"
#include "NetSession.h"

/*  只重写onMsg--使用websocket协议,数据使用二进制packet协议作为通信的基础网络会话  */

class UseWebPacketSingleNetSession : public BaseNetSession, public std::enable_shared_from_this<UseWebPacketSingleNetSession>
{
public:
    UseWebPacketSingleNetSession();

private:
    virtual size_t  onMsg(const char* buffer, size_t len) final;
    virtual void    procPacket(PACKET_OP_TYPE op, const char* body, PACKET_LEN_TYPE bodyLen) = 0;

private:
    bool            mShakehanded;   /*  是否已经握手  */
};

#endif