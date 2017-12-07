#include "NetThreadSession.h"

brynet::MsgQueue<Net2LogicMsg>  gNet2LogicMsgList;
std::mutex              gNet2LogicMsgListLock;

void ExtNetSession::pushDataMsgToLogicThread(const char* data, size_t len)
{
    pushDataMsg2LogicMsgList(mLogicSession, data, len);
}

void ExtNetSession::onEnter()
{
    std::lock_guard<std::mutex> lck(gNet2LogicMsgListLock);
    mLogicSession->setSession(getService()->getService(), getSocketID(), getIP());
    Net2LogicMsg tmp(mLogicSession, Net2LogicMsgType::Net2LogicMsgTypeEnter);
    gNet2LogicMsgList.push(tmp);
}

void ExtNetSession::onClose()
{
    std::lock_guard<std::mutex> lck(gNet2LogicMsgListLock);
    Net2LogicMsg tmp(mLogicSession, Net2LogicMsgType::Net2LogicMsgTypeClose);
    gNet2LogicMsgList.push(tmp);
}

void pushDataMsg2LogicMsgList(BaseLogicSession::PTR session, const char* data, size_t len)
{
    std::lock_guard<std::mutex> lck(gNet2LogicMsgListLock);
    Net2LogicMsg tmp(session, Net2LogicMsgType::Net2LogicMsgTypeData);
    tmp.setData(data, len);
    gNet2LogicMsgList.push(tmp);
}

void pushCompleteCallback2LogicMsgList(const DataSocket::PACKED_SENDED_CALLBACK& callback)
{
    std::lock_guard<std::mutex> lck(gNet2LogicMsgListLock);
    Net2LogicMsg tmp(nullptr, Net2LogicMsgType::Net2LogicMsgTypeCompleteCallback);
    tmp.mSendCompleteCallback = callback;
    gNet2LogicMsgList.push(tmp);
}

void syncNet2LogicMsgList(std::shared_ptr<EventLoop> eventLoop)
{
    gNet2LogicMsgListLock.lock();
    gNet2LogicMsgList.forceSyncWrite();
    gNet2LogicMsgListLock.unlock();
    if (gNet2LogicMsgList.sharedListSize() > 0)
    {
        eventLoop->wakeup();
    }
}

void procNet2LogicMsgList()
{
    gNet2LogicMsgList.syncRead(std::chrono::microseconds::zero());

    Net2LogicMsg msg;
    while (gNet2LogicMsgList.popFront(msg))
    {
        switch (msg.mMsgType)
        {
            case Net2LogicMsgType::Net2LogicMsgTypeEnter:
            {
                msg.mSession->onEnter();
            }
            break;
            case Net2LogicMsgType::Net2LogicMsgTypeData:
            {
                msg.mSession->onMsg(msg.mPacket.c_str(), msg.mPacket.size());
            }
            break;
            case Net2LogicMsgType::Net2LogicMsgTypeClose:
            {
                msg.mSession->onClose();
            }
            break;
            case Net2LogicMsgType::Net2LogicMsgTypeCompleteCallback:
            {
                (*msg.mSendCompleteCallback)();
            }
            break;
            default:
                assert(false);
                break;
        }
    }
}