#ifndef _CLIENT_SESSION_H
#define _CLIENT_SESSION_H

#include "NetSession.h"
#include "UsePacketSingleNetSession.h"
#include "UseWebPacketSingleNetSession.h"
#include "LogicServerSession.h"

/*  客户端链接会话(客户端网络掉线，此对象就销毁)    */
class ConnectionClientSession : public UseWebPacketSingleNetSession
{
public:
    typedef std::shared_ptr<ConnectionClientSession> PTR;

    ConnectionClientSession();
    ~ConnectionClientSession();

    void                setSlaveServerID(int id);
    int                 getPrimaryServerID() const;
    int64_t             getRuntimeID() const;

    void                sendPBBinary(int32_t cmd, const char* data, size_t len);

private:
    virtual void        onEnter() override;
    virtual void        onClose() override;
    virtual void        procPacket(uint32_t op, const char* body, uint32_t bodyLen);

    void                claimPrimaryServer();
    void                claimRuntimeID();

private:
    int64_t             mRuntimeID;
    int                 mPrimaryServerID;
    int                 mSlaveServerID;

    int32_t             mRecvSerialID;
    int32_t             mSendSerialID;

    LogicServerSession::PTR mPrimaryServer;
    LogicServerSession::PTR mSlaveServer;
};

#endif