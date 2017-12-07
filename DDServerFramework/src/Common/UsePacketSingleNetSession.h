#ifndef _USE_PACKET_SINGLE_EXENETSESSION_H
#define _USE_PACKET_SINGLE_EXENETSESSION_H

#include <memory>
#include <brynet/utils/packet.h>
#include <brynet/net/NetSession.h>

/*  ֻ��дonMsg--ʹ�ö�����packetЭ����Ϊͨ�ŵĻ�������Ự  */
/* TODO::�����д���л������麯��,����������Ŀǰ�ķ����л�   */
/* TODO::��procPacketǩ���ĵ�,ȥ����������Ͳ���  */

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