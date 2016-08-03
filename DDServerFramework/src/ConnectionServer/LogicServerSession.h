#ifndef _LOGIC_SERVER_SESSION_H
#define _LOGIC_SERVER_SESSION_H

#include "NetSession.h"
#include "UsePacketSingleNetSession.h"

class ReadPacket;

/*逻辑服务器链接*/
class LogicServerSession : public UsePacketSingleNetSession
{
public:
    LogicServerSession();

private:
    virtual     void    onEnter() override;
    virtual     void    onClose() override;

    void                procPacket(PACKET_OP_TYPE op, const char* body, PACKET_LEN_TYPE bodyLen);

private:
    /*内部服务器登陆此链接服务器*/
    void                onLogicServerLogin(ReadPacket& rp);
    /*内部服务器请求转发消息给客户端(以SocketID标识)*/
    void                onPacket2ClientBySocketInfo(ReadPacket& rp);
    /*内部服务器请求转发消息给客户端(以RuntimeID标识)*/
    void                onPacket2ClientByRuntimeID(ReadPacket& rp);

    void                onSlaveServerIsSetClient(ReadPacket& rp);
    /*强制踢出某RuntimeID所标识的客户端*/
    void                onKickClientByRuntimeID(ReadPacket& rp);

    void                onPing(ReadPacket& rp);

private:
    bool                checkPassword(const string& password);
    void                sendLogicServerLoginResult(bool isSuccess, const string& reason);

private:
    bool                mIsPrimary;
    int                 mID;
};

void    AddPrimaryLogicServer(int id, BaseNetSession::PTR);
BaseNetSession::PTR FindPrimaryLogicServer(int id);
void    RemovePrimaryLogicServer(int id);
int     ClaimPrimaryLogicServer();

void    AddSlaveLogicServer(int id, BaseNetSession::PTR);
BaseNetSession::PTR FindSlaveLogicServer(int id);
void    RemoveSlaveLogicServer(int id);

#endif