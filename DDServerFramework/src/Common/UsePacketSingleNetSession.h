#ifndef _USE_PACKET_SINGLE_EXENETSESSION_H
#define _USE_PACKET_SINGLE_EXENETSESSION_H

#include <memory>
#include <brynet/utils/packet.h>
#include <brynet/net/NetSession.h>

/*  只重写onMsg--使用二进制packet协议作为通信的基础网络会话  */
/* TODO::添加重写序列化包的虚函数,而不仅仅是目前的反序列化   */
/* TODO::将procPacket签名改掉,去掉具体的类型参数  */

class UsePacketSingleNetSession : public brynet::net::BaseNetSession, public std::enable_shared_from_this<UsePacketSingleNetSession>
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