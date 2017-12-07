#ifndef _CLIENT_SESSION_H
#define _CLIENT_SESSION_H

#include <brynet/net/NetSession.h>
#include "UsePacketSingleNetSession.h"
#include "LogicServerSession.h"

/*  �ͻ������ӻỰ(�ͻ���������ߣ��˶��������)    */
class ConnectionClientSession : public UsePacketSingleNetSession
{
public:
    typedef std::shared_ptr<ConnectionClientSession> PTR;

    ConnectionClientSession(int32_t connectionServerID);
    ~ConnectionClientSession();

    void                setSlaveServerID(int id);
    int                 getSlaveServerID() const;
    void                setPrimaryServerID(int);
    int                 getPrimaryServerID() const;
    int64_t             getRuntimeID() const;
    void                notifyExit();

private:
    virtual void        onEnter() override;
    virtual void        onClose() override;
    virtual void        procPacket(PACKET_OP_TYPE op, const char* body, PACKET_LEN_TYPE bodyLen);

    void                claimPrimaryServer();

private:
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