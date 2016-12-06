#ifndef _CONNECTIONSERVER_CONNECTION_H
#define _CONNECTIONSERVER_CONNECTION_H

#include <memory>
#include <unordered_map>
#include <stdint.h>

#include "LogicNetSession.h"

class Packet;

/*链接到链接服务器*/
class ConnectionServerConnection : public BaseLogicSession, public std::enable_shared_from_this<ConnectionServerConnection>
{
public:
    typedef std::shared_ptr<ConnectionServerConnection> PTR;
    typedef std::weak_ptr<ConnectionServerConnection> WEAK_PTR;

    ConnectionServerConnection(int idInEtcd, int port, std::string password);
    ~ConnectionServerConnection();

    void            sendPacket(Packet&);

private:
    virtual void    onEnter() final;
    virtual void    onClose() final;
    virtual void    onMsg(const char* data, size_t len) final;

    void            ping();

private:
    bool            mIsSuccess;
    int             mPort;
    int             mIDInEtcd;
    int32_t         mConnectionServerID;
    std::string     mPassword;
    dodo::Timer::WeakPtr  mPingTimer;
};

extern std::unordered_map<int32_t, ConnectionServerConnection::PTR> gAllLogicConnectionServerClient;

void    tryCompareConnect(std::unordered_map<int32_t, std::tuple<std::string, int, std::string>>& servers);

#endif