#ifndef _CENTERSERVER_CONNECTION_H
#define _CENTERSERVER_CONNECTION_H

#include <stdint.h>
#include <functional>
#include <memory>

#include "LogicNetSession.h"
#include "MsgpackRpc.h"
#include "RpcService.h"
#include "packet.h"
#include "CenterServerRecvOP.h"

class Packet;
class ReadPacket;

/*中心服务器链接*/
class CenterServerConnection : public BaseLogicSession
{
public:
    typedef std::shared_ptr<CenterServerConnection> PTR;
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
        std::string rpcstr = gCenterServerConnectioinRpc.call(funname, args...);
        sendv(static_cast<PACKET_OP_TYPE>(CENTER_SERVER_RECV_OP::CENTERSERVER_RECV_OP_RPC), rpcstr);
    }

    template<typename... Args>
    void            reply(dodo::rpc::RpcRequestInfo& info, const Args&... args)
    {
        if (info.getRequestID() != -1)
        {
            std::string rpcstr = gCenterServerConnectioinRpc.reply(info.getRequestID(), args...);
            sendv(static_cast<PACKET_OP_TYPE>(CENTER_SERVER_RECV_OP::CENTERSERVER_RECV_OP_RPC), rpcstr);

            info.setRequestID(-1);
        }
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

private:
    void            ping();
    void            onCenterServerSendPacket2Client(ReadPacket& rp);
    void            onUserMsg(ReadPacket& rp);

private:
    int32_t         mSelfID;
    brynet::Timer::WeakPtr  mPingTimer;

private:
    static std::unordered_map<PACKET_OP_TYPE, USER_MSG_HANDLE>  sUserMsgHandlers;
};

#endif