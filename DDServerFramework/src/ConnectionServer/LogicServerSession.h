#ifndef _LOGIC_SERVER_SESSION_H
#define _LOGIC_SERVER_SESSION_H

#include <memory>
#include <string>

#include <brynet/net/NetSession.h>
#include "UsePacketSingleNetSession.h"

class ReadPacket;

/*  �߼����������� */
class LogicServerSession : public UsePacketSingleNetSession
{
public:
    typedef std::shared_ptr<LogicServerSession> PTR;

    LogicServerSession(int32_t connectionServerID, std::string password);
    int                 getID() const;

private:
    virtual     void    onEnter() override;
    virtual     void    onClose() override;

    void                procPacket(PACKET_OP_TYPE op, const char* body, PACKET_LEN_TYPE bodyLen);

private:
    /*  �ڲ���������½�����ӷ�����   */
    void                onLogicServerLogin(ReadPacket& rp);
    /*  �ڲ�����������ת����Ϣ���ͻ���(��RuntimeID��ʶ) */
    void                onPacket2ClientByRuntimeID(ReadPacket& rp);
    void                onPacket2ClientBySocketInfo(ReadPacket& rp);

    void                onPrimaryServerIsSetClient(ReadPacket& rp);
    void                onSlaveServerIsSetClient(ReadPacket& rp);
    /*  ǿ���߳�ĳRuntimeID����ʶ�Ŀͻ���   */
    void                onKickClientByRuntimeID(ReadPacket& rp);
    void                onPing(ReadPacket& rp);

private:
    bool                checkPassword(const std::string& password);
    void                sendLogicServerLoginResult(bool isSuccess, const std::string& reason);

private:
    const int32_t       mConnectionServerID;
    const std::string   mPassword;

    bool                mIsPrimary;
    int                 mID;
};

#endif