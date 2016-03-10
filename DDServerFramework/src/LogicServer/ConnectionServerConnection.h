#ifndef _CONNECTIONSERVER_CONNECTION_H
#define _CONNECTIONSERVER_CONNECTION_H

#include <unordered_map>
#include <stdint.h>

#include "LogicNetSession.h"

class WrapServer;
class Packet;

/*链接到链接服务器*/
class ConnectionServerConnection : public BaseLogicSession
{
public:
    ConnectionServerConnection(int idInEtcd, int port);
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
    Timer::WeakPtr  mPingTimer;
};

extern std::unordered_map<int32_t, ConnectionServerConnection*> gAllLogicConnectionServerClient;

extern int gSelfID;
extern bool gIsPrimary;

void    tryCompareConnect(std::unordered_map<int32_t, std::tuple<std::string, int>>& servers);

#endif