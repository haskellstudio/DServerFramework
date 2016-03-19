#include "packet.h"
#include "WrapLog.h"

#include "ClientLogicObject.h"
#include "ClientSession.h"

extern WrapLog::PTR gDailyLogger;

ConnectionClientSession::ConnectionClientSession()
{
    gDailyLogger->info("ConnectionClientSession : {}, {}", getSocketID(), getIP());
    mClient = nullptr;
    mPingCount = 0;
}

ConnectionClientSession::~ConnectionClientSession()
{
    gDailyLogger->info("~ ConnectionClientSession : {}, {}", getSocketID(), getIP());
}

ClientObject::PTR ConnectionClientSession::getClientObject()
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
    /*  TODO::当此网络对象断开时，所在的逻辑客户端对象开启断线重连的等待定时器*/
    gDailyLogger->warn("client close, ip:{}, socket id :{}", getIP(), getSocketID());
    if (true)
    {
        gDailyLogger->warn("do not wait re connect, runtime id:{}", mClient->getRuntimeID());
        eraseClientByRuntimeID(mClient->getRuntimeID());
    }
    else
    {
        mClient->startDelayWait();
        mClient->notifyDisConnect();
    }
}

//  断线重连由客户端发送user msg来做，然后再经由logic server反向让connection server来操作

///*  客户端请求断线重连(新启动的一个链接，请求重连之前的某一个clientObject    */
//void ConnectionclientSession::reConnect(const char* packerBuffer, PACKET_OP_TYPE packetLen)
//{
//    /*如果此网络链接还没有分配RuntimeID，也即还没在内部服务器分配任何资源*/
//    if (mclient->getRuntimeID() == -1)
//    {
//        /*TODO::重连需要验证合法性，避免随意重连到某一个断开的客户端对象上*/
//        ReadPacket rp(packerBuffer, packetLen);
//        rp.readPacketLen();
//        rp.readOP();
//        /*要重连的客户端的RuntimeID*/
//        int64_t oldRuntimeID = rp.readINT64();
//        ClientObject::PTR oldClient = findClientByRuntimeID(oldRuntimeID);
//        if (oldClient != nullptr && oldClient->isInDelayWait())
//        {
//            mClient = oldClient;
//            mClient->cancelDelayTimer();
//            mClient->resetSocketID(getSocketID());
//            mClient->notifyReConnect();
//        }
//    }
//}
