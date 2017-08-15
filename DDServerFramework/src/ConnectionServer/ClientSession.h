#ifndef _CLIENT_SESSION_H
#define _CLIENT_SESSION_H

#include "NetSession.h"
#include "UsePacketSingleNetSession.h"
#include "LogicServerSession.h"

/*  客户端链接会话(客户端网络掉线，此对象就销毁)    */
class ConnectionClientSession : public UsePacketSingleNetSession
{
public:
    typedef std::shared_ptr<ConnectionClientSession> PTR;

    ConnectionClientSession(int32_t connectionServerID);
    ~ConnectionClientSession();

    void                setSlaveServerID(int id);
    int                 getPrimaryServerID() const;
    int64_t             getRuntimeID() const;

private:
    virtual void        onEnter() override;
    virtual void        onClose() override;
    virtual void        procPacket(PACKET_OP_TYPE op, const char* body, PACKET_LEN_TYPE bodyLen);

    void                claimPrimaryServer();
    void                claimRuntimeID();

private:
    const int32_t       mConnectionServerID;
    int64_t             mRuntimeID;
    int                 mPrimaryServerID;
    int                 mSlaveServerID;

    int32_t             mRecvSerialID;
    int32_t             mSendSerialID;

    LogicServerSession::PTR mPrimaryServer;
    LogicServerSession::PTR mSlaveServer;
};

#endif