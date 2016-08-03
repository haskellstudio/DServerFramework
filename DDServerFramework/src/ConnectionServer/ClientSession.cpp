#include "packet.h"
#include "WrapLog.h"

#include "ClientLogicObject.h"
#include "ClientSession.h"

extern WrapLog::PTR gDailyLogger;

ConnectionClientSession::ConnectionClientSession()
{
    gDailyLogger->info("ConnectionClientSession : {}, {}", getSocketID(), getIP());
    mClient = nullptr;
}

ConnectionClientSession::~ConnectionClientSession()
{
    gDailyLogger->info("~ ConnectionClientSession : {}, {}", getSocketID(), getIP());
}

ClientObject::PTR& ConnectionClientSession::getClientObject()
{
    return mClient;
}

void ConnectionClientSession::procPacket(PACKET_OP_TYPE op, const char* body, PACKET_LEN_TYPE bodyLen)
{
    mClient->procPacket(op, body - PACKET_HEAD_LEN, bodyLen + PACKET_HEAD_LEN);
}

void ConnectionClientSession::onEnter()
{
    gDailyLogger->warn("client enter, ip:{}, socket id :{}", getIP(), getSocketID());
    mClient = std::make_shared<ClientObject>(getSocketID());
}

void ConnectionClientSession::onClose()
{
    gDailyLogger->warn("client close, ip:{}, socket id :{}", getIP(), getSocketID());
    eraseClientByRuntimeID(mClient->getRuntimeID());
}