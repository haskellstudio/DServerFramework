#ifndef _CENTER_SERVER_CLIENT_H
#define _CENTER_SERVER_CLIENT_H

#include <unordered_map>
#include <memory>
#include <string>
#include <stdint.h>

#include "LogicNetSession.h"
#include "memberrpc.h"
#include "packet.h"
#include "CenterServerSendOP.h"

class Packet;
class ReadPacket;

/*内部服务器链接中心服务器的会话*/

class CenterServerSession : public BaseLogicSession, public std::enable_shared_from_this<CenterServerSession>
{
public:
    typedef std::shared_ptr<CenterServerSession> PTR;

    CenterServerSession();

    typedef std::function<void(CenterServerSession::PTR&, ReadPacket& rp)>   USER_MSG_HANDLE;

    int             getID() const;

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

    template<typename... Args>
    void            reply(dodo::rpc::RpcRequestInfo& info, const Args&... args)
    {
        if (info.getRequestID() != -1)
        {
            string rpcstr = CenterServerSessionGlobalData::getCenterServerSessionRpc()->reply(info.getRequestID(), args...);
            sendv(CENTERSERVER_SEND_OP_RPC, rpcstr);

            info.setRequestID(-1);
        }
    }

    template<typename... Args>
    void            call(const std::string& funname, const Args&... args)
    {
        string rpcstr = CenterServerSessionGlobalData::getCenterServerSessionRpc().call(funname.c_str(), args...);
        sendv(CENTERSERVER_SEND_OP_RPC, rpcstr);
    }

    template<typename PBType, typename... Args>
    void            callPB(const PBType& pb, const Args&... args)
    {
        call(std::remove_reference<PBType>::type::descriptor()->full_name().c_str(), pb, args...);
    }

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

    static  std::shared_ptr<dodo::rpc::RpcService < dodo::rpc::MsgpackProtocol>>&    getCenterServerSessionRpc();

    static  void                        registerUserMsgHandle(PACKET_OP_TYPE op, CenterServerSession::USER_MSG_HANDLE handle);
    static  CenterServerSession::USER_MSG_HANDLE    findUserMsgHandle(PACKET_OP_TYPE op);

    template<typename F>
    static void        def(const char* funname, F func)
    {
        regFunctor(funname, func);
    }

private:
    template<typename RET>
    class FuckYou
    {
    public:
        template<typename FUNC, typename RET, typename ...Args>
        static void _insertLambdahelp(std::string name, FUNC func)
        {
            sCenterServerSessionRpc->def(name.c_str(), [func](const Args&... args, dodo::rpc::RpcRequestInfo info){
                auto r = func(args...);
                sCenterServerSessionRpcFromer->reply(info, r);
            });
        }
    };

    template<>
    class FuckYou<void>
    {
    public:
        template<typename FUNC, typename RET, typename ...Args>
        static void _insertLambdahelp(std::string name, FUNC func)
        {
            sCenterServerSessionRpc->def(name.c_str(), [func](const Args&... args, dodo::rpc::RpcRequestInfo info){
                func(args...);
            });
        }
    };

    template<typename RET, typename ...Args>
    static void regFunctor(const char* funname, RET(*func)(Args...))
    {
        std::function<RET(Args...)> tmpf = func;
        regFunctor(funname, tmpf);
    }

    template<typename FUNTYPE>
    static void regFunctor(const char* funname, std::function<FUNTYPE> func)
    {
        _insertLambda<std::function<FUNTYPE>, std::function<FUNTYPE>::result_type>(funname, func, &std::function<FUNTYPE>::operator());
    }

    template<typename LAMBDA>
    static void regFunctor(const char* funname, LAMBDA lambdaObj)
    {
        _insertLambda(funname, lambdaObj, &LAMBDA::operator());
    }

    template<typename LAMBDA_OBJ_TYPE, typename RET, typename ...Args>
    static void _insertLambda(std::string name, LAMBDA_OBJ_TYPE obj, RET(LAMBDA_OBJ_TYPE::*func)(Args...) const)
    {
        FuckYou<RET>::_insertLambdahelp<LAMBDA_OBJ_TYPE, RET, Args...>(name, obj);
    }
private:
    static  std::unordered_map<int, CenterServerSession::PTR>    sAllLogicServer;
    static  std::shared_ptr<dodo::rpc::RpcService < dodo::rpc::MsgpackProtocol>>  sCenterServerSessionRpc;
    static  CenterServerSession::PTR                             sCenterServerSessionRpcFromer;
    static std::unordered_map<PACKET_OP_TYPE, CenterServerSession::USER_MSG_HANDLE> sUserMsgHandles;
};

typedef CenterServerSessionGlobalData CenterServerRPCMgr;

#endif