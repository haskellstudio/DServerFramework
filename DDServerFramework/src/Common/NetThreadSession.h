#ifndef _NETTHREAD_SESSION_H
#define _NETTHREAD_SESSION_H

#include <string>
#include <memory>

#include <brynet/utils/MsgQueue.h>
#include <brynet/net/NetSession.h>
#include <brynet/net/EventLoop.h>
#include <brynet/net/DataSocket.h>

#include "LogicNetSession.h"

/*������㣨�̣߳��¼�ת��Ϊ��Ϣ��������Ϣ���У��ṩ���߼��̴߳���*/

/*������Ϣ����*/
enum class Net2LogicMsgType
{
    Net2LogicMsgTypeNONE,
    Net2LogicMsgTypeEnter,
    Net2LogicMsgTypeData,
    Net2LogicMsgTypeClose,
    Net2LogicMsgTypeCompleteCallback,
};

/*�����̵߳��߼��̵߳�������Ϣ�ṹ*/
class Net2LogicMsg
{
public:
    Net2LogicMsg(){}

    Net2LogicMsg(BaseLogicSession::PTR session, Net2LogicMsgType msgType) : mSession(session), mMsgType(msgType)
    {}

    Net2LogicMsg(Net2LogicMsg&& outher) : mSession(outher.mSession), mMsgType(outher.mMsgType), mPacket(std::move(outher.mPacket)), mSendCompleteCallback(std::move(outher.mSendCompleteCallback))
    {
    }

    Net2LogicMsg(const Net2LogicMsg& right) : mSession(right.mSession), mMsgType(right.mMsgType), mPacket(right.mPacket), mSendCompleteCallback(right.mSendCompleteCallback)
    {
    }

    Net2LogicMsg& operator =(Net2LogicMsg&& outher)
    {
        if (this != &outher)
        {
            mSession = outher.mSession;
            mMsgType = outher.mMsgType;
            mPacket = std::move(outher.mPacket);
            mSendCompleteCallback = std::move(outher.mSendCompleteCallback);
        }

        return *this;
    }

    void                setData(const char* data, size_t len)
    {
        mPacket.append(data, len);
    }

    BaseLogicSession::PTR   mSession;
    Net2LogicMsgType        mMsgType;
    std::string             mPacket;
    DataSocket::PACKED_SENDED_CALLBACK mSendCompleteCallback;
};

/*  ��չ���Զ�������Ự�������¼���������ṹ����Ϣ���͵��߼��߳�    */
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
void syncNet2LogicMsgList(std::shared_ptr<EventLoop> eventLoop);

void procNet2LogicMsgList();    /*���߳�,���̰߳�ȫ*/

#endif
