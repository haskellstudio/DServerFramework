#ifndef _CLIENT_SESSION_H
#define _CLIENT_SESSION_H

#include <brynet/net/NetSession.h>
#include "LogicServerSession.h"

/*  客户端链接会话(客户端网络掉线，此对象就销毁)    */
class ConnectionClientSession : std::enable_shared_from_this<ConnectionClientSession>
{
public:
    typedef std::shared_ptr<ConnectionClientSession> PTR;

    ConnectionClientSession(int32_t connectionServerID, int64_t socketID, std::string ip);
    ~ConnectionClientSession();

    void                setSlaveServerID(int id);
    int                 getSlaveServerID() const;
    void                setPrimaryServerID(int);
    int                 getPrimaryServerID() const;
    int64_t             getRuntimeID() const;
    void                notifyExit();

public:
    void                onEnter();
    void                onClose();
    void                procPacket(PACKET_OP_TYPE op, const char* body, PACKET_LEN_TYPE bodyLen);

    void                claimPrimaryServer();

private:
    int64_t             getSocketID() const;
    const std::string&  getIP() const;

private:
    const int64_t       mSocketID;
    const std::string   mIP;
    const int32_t       mConnectionServerID;
    int64_t             mRuntimeID;
    int                 mPrimaryServerID;
    int                 mSlaveServerID;
    bool                mReconnecting;

    int32_t             mRecvSerialID;
    int32_t             mSendSerialID;

    LogicServerSession::PTR mPrimaryServer;
    LogicServerSession::PTR mSlaveServer;
};

#endif