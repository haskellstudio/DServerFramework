#ifndef _CENTER_SERVER_CLIENT_H
#define _CENTER_SERVER_CLIENT_H

#include <unordered_map>
#include <memory>
#include <stdint.h>

#include "LogicNetSession.h"
#include "memberrpc.h"
#include "packet.h"

class WrapServer;
class Packet;
class ReadPacket;

/*内部服务器链接中心服务器的会话*/

class CenterServerSession : public BaseLogicSession, public std::enable_shared_from_this<CenterServerSession>
{
public:
    typedef std::shared_ptr<CenterServerSession> PTR;

    CenterServerSession();

    typedef std::function<void(CenterServerSession&, ReadPacket& rp)>   USER_MSG_HANDLE;

    void            sendPacket(Packet&);

    template<typename... Args>
    void            sendv(PACKET_OP_TYPE op, const Args&... args)
    {
        BigPacket packet(op);
        packet.writev(args...);
        sendPacket(packet);
    }

    void            sendUserPacket(Packet&);

    template<typename... Args>
    void            sendUserMsgV(PACKET_OP_TYPE op, const Args&... args)
    {
        BigPacket packet(op);
        packet.writev(args...);

        sendUserPacket(packet);
    }

    void            sendPacket2Client(int64_t runtimeID, Packet& realPacket);

private:
    virtual void    onEnter() final;
    virtual void    onClose() final;
    virtual void    onMsg(const char* data, size_t len) final;

private:
    void            onPing(ReadPacket& rp);
    void            onLogicServerRpc(ReadPacket& rp);
    /*内部逻辑服务器登陆*/
    void            onLogicServerLogin(ReadPacket& rp);
    void            onUserMsg(ReadPacket& rp);

private:
    int             mID;
};

class CenterServerSessionGlobalData
{
public:
    static  void                        init();
    static  void                        destroy();

    static  CenterServerSession::PTR    findLogicServer(int id);
    static  void                        removeLogicServer(int id);
    static  void                        insertLogicServer(CenterServerSession::PTR, int id);

    static  CenterServerSession::PTR&   getRpcFromer();
    static  void                        setRpcFrommer(CenterServerSession::PTR);

    static  std::shared_ptr<dodo::rpc < dodo::MsgpackProtocol>>&    getCenterServerSessionRpc();

    static  void                        registerUserMsgHandle(PACKET_OP_TYPE op, CenterServerSession::USER_MSG_HANDLE handle);
    static  CenterServerSession::USER_MSG_HANDLE    findUserMsgHandle(PACKET_OP_TYPE op);
private:
    static  std::unordered_map<int, CenterServerSession::PTR>    sAllLogicServer;
    static  std::shared_ptr<dodo::rpc < dodo::MsgpackProtocol>>  sCenterServerSessionRpc;
    static  CenterServerSession::PTR                             sCenterServerSessionRpcFromer;
    static std::unordered_map<PACKET_OP_TYPE, CenterServerSession::USER_MSG_HANDLE> sUserMsgHandles;
};

#endif