#ifndef _CLIENT_SESSION_H
#define _CLIENT_SESSION_H

#include <atomic>

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

    void                setPrimaryServerID(int id);
    void                setSlaveServerID(int id);

    int                 getPrimaryServerID() const;
    int64_t             getRuntimeID() const;

    void                notifyServerPlayerExist();

    void                sendPBBinary(int32_t cmd, const char* data, size_t len);
    void                sendPBBinary(int32_t cmd, std::shared_ptr<std::string>& data);
    void                sendPBBinary(int32_t cmd, std::shared_ptr<std::string>&& data);

private:
    virtual void        onEnter() override;
    virtual void        onClose() override;
    virtual void        procPacket(uint32_t op, const char* body, uint32_t bodyLen);

    void                claimPrimaryServer();

    void                helpSendPacket(uint32_t op, const char* data, size_t len);
private:
    int64_t             mRuntimeID;

    int32_t                 mRecvSerialID;
    std::atomic<int32_t>    mSendSerialID;

    LogicServerSession::PTR mPrimaryServer;
    LogicServerSession::PTR mSlaveServer;

    bool                    mReconnecting;
};

#endif