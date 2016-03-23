#ifndef _NETTHREAD_SESSION_H
#define _NETTHREAD_SESSION_H

#include <string>

#include "msgqueue.h"
#include "LogicNetSession.h"
#include "NetSession.h"
#include "EventLoop.h"

using namespace std;

/*将网络层（线程）事件转换为消息，放入消息队列，提供给逻辑线程处理*/

/*网络消息类型*/
enum Net2LogicMsgType
{
    Net2LogicMsgTypeNONE,
    Net2LogicMsgTypeEnter,
    Net2LogicMsgTypeData,
    Net2LogicMsgTypeClose,
    Net2LogicMsgTypeCompleteCallback,
};

/*网络线程到逻辑线程的网络消息结构*/
class Net2LogicMsg
{
public:
    Net2LogicMsg(){}

    Net2LogicMsg(BaseLogicSession::PTR session, Net2LogicMsgType msgType) : mSession(session), mMsgType(msgType)
    {}

    void                setData(const char* data, size_t len)
    {
        mPacket.append(data, len);
    }

    BaseLogicSession::PTR   mSession;
    Net2LogicMsgType        mMsgType;
    std::string             mPacket;
    DataSocket::PACKED_SENDED_CALLBACK mSendCompleteCallback;
};

/*  扩展的自定义网络会话对象，其事件处理函数里会构造消息发送到逻辑线程    */
class ExtNetSession : public BaseNetSession
{
public:
    ExtNetSession(BaseLogicSession::PTR logicSession) : BaseNetSession(), mLogicSession(logicSession)
    {
    }

protected:
    void            pushDataMsgToLogicThread(const char* data, size_t len);
private:
    virtual void    onEnter() final;
    virtual void    onClose() final;

protected:
    BaseLogicSession::PTR   mLogicSession;
};

void pushDataMsg2LogicMsgList(BaseLogicSession::PTR, const char* data, size_t len);
void pushCompleteCallback2LogicMsgList(const DataSocket::PACKED_SENDED_CALLBACK& callback);
void syncNet2LogicMsgList(EventLoop& eventLoop);

void procNet2LogicMsgList();    /*单线程,非线程安全*/

#endif
