#ifndef _CENTERSERVER_CONNECTION_H
#define _CENTERSERVER_CONNECTION_H

#include <stdint.h>
#include <functional>

#include "LogicNetSession.h"
#include "msgpackrpc.h"
#include "drpc.h"
#include "packet.h"
#include "CenterServerRecvOP.h"

class WrapServer;
class Packet;
class ReadPacket;

/*链接到中心服务器*/
class CenterServerConnection : public BaseLogicSession
{
public:
    typedef std::function<void(CenterServerConnection&, ReadPacket&)>   USER_MSG_HANDLE;

    CenterServerConnection();
    ~CenterServerConnection();

    void            sendPacket(Packet&);
    template<typename... Args>
    void            sendv(PACKET_OP_TYPE op, const Args&... args)
    {
        BigPacket packet(op);
        packet.writev(args...);
        sendPacket(packet);
    }

    void            sendUserPacket(Packet& subPacket);
    template<typename... Args>
    void            sendUserV(PACKET_OP_TYPE op, const Args&... args)
    {
        BigPacket packet(op);
        packet.writev(args...);
        sendUserPacket(packet);
    }

    int32_t         getSelfID() const;

    template<typename... Args>
    void            call(const char* funname, const Args&... args)
    {
        string tmp = gCenterServerConnectioinRpc.call(funname, args...);
        BigPacket p(CENTERSERVER_RECV_OP_RPC);
        p.writeBinary(tmp);
        sendPacket(p);
    }

    template<typename PBType, typename... Args>
    void            callPB(const PBType& pb, const Args&... args)
    {
        call(std::remove_reference<PBType>::type::descriptor()->full_name().c_str(), pb, args...);
    }

    static void     registerUserMsgHandle(PACKET_OP_TYPE, USER_MSG_HANDLE);

private:
    virtual void    onEnter() final;
    virtual void    onClose() final;
    virtual void    onMsg(const char* buffer, size_t len) final;

    void            ping();

private:
    void            onCenterServerSendPacket2Client(ReadPacket& rp);
    void            onUserMsg(ReadPacket& rp);

private:
    int32_t         mSelfID;
    Timer::WeakPtr  mPingTimer;

private:
    static std::unordered_map<PACKET_OP_TYPE, USER_MSG_HANDLE>  sUserMsgHandlers;
};

extern CenterServerConnection*              gLogicCenterServerClient;
extern dodo::rpc<dodo::MsgpackProtocol>     gCenterServerConnectioinRpc;

#endif