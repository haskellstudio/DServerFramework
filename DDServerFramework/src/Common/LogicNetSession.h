#ifndef _LOGIC_NET_SESSION_H
#define _LOGIC_NET_SESSION_H

#include <stdint.h>
#include <memory>

#include <brynet/net/WrapTCPService.h>

using namespace brynet::net;

class BaseLogicSession : public std::enable_shared_from_this<BaseLogicSession>
{
public:
    typedef std::shared_ptr<BaseLogicSession> PTR;

    BaseLogicSession()
    {
    }

    virtual ~BaseLogicSession()
    {}

    void            setSession(TcpService::PTR server, int64_t socketID, const std::string& ip)
    {
        mServer = server;
        mSocketID = socketID;
        mIP = ip;
    }

    virtual void    onEnter() = 0;
    virtual void    onClose() = 0;
    virtual void    onMsg(const char* buffer, size_t len) = 0;

    void            send(const char* buffer, size_t len, const DataSocket::PACKED_SENDED_CALLBACK& callback = nullptr)
    {
        mServer->send(mSocketID, DataSocket::makePacket(buffer, len), callback);
    }

    void            send(const DataSocket::PACKET_PTR& packet, const DataSocket::PACKED_SENDED_CALLBACK& callback = nullptr)
    {
        mServer->send(mSocketID, packet, callback);
    }

    int64_t         getSocketID() const
    {
        return mSocketID;
    }

    std::string         getIP()
    {
        return mIP;
    }

private:
    TcpService::PTR     mServer;
    int64_t             mSocketID;
    std::string         mIP;
};

#endif