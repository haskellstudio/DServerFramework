#ifndef _LOGIC_SERVER_SESSION_H
#define _LOGIC_SERVER_SESSION_H

#include <memory>
#include <string>
#include "NetSession.h"
#include "UseCellnetSingleNetSession.h"

class BasePacketReader;

/*  逻辑服务器链接 */
class LogicServerSession : public UseCellnetPacketSingleNetSession
{
public:
    typedef std::shared_ptr<LogicServerSession> PTR;

    LogicServerSession();

    void                sendPBData(uint32_t cmd, const char* data, size_t len);

    template<typename T>
    void                sendPB(uint32_t cmd, const T& t)
    {
        char buff[8 * 1024];
        if (t.SerializeToArray((void*)buff, t.ByteSize()))
        {
            sendPBData(cmd, buff, t.ByteSize());
        }
    }

private:
    virtual     void    onEnter() override;
    virtual     void    onClose() override;

    void                procPacket(uint32_t op, const char* body, uint16_t bodyLen);

private:
    /*  内部服务器登陆此链接服务器   */
    void                onLogicServerLogin(BasePacketReader& rp);
    /*  内部服务器请求转发消息给客户端(以RuntimeID标识) */
    void                onPacket2ClientByRuntimeID(BasePacketReader& rp);

    void                onSlaveServerIsSetClient(BasePacketReader& rp);
    /*  强制踢出某RuntimeID所标识的客户端   */
    void                onKickClientByRuntimeID(BasePacketReader& rp);

private:
    bool                checkPassword(const std::string& password);
    void                sendLogicServerLoginResult(bool isSuccess, const std::string& reason);

private:
    bool                mIsPrimary;
    int                 mID;

    uint16_t            mSendSerialID;
};

#endif