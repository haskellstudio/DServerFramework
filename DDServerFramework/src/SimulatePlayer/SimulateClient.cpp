#include <set>
#include <string>
#include <iostream>

using namespace std;

#include "NetSession.h"
#include "WrapTCPService.h"
#include "socketlibfunction.h"
#include "platform.h"
#include "packet.h"
#include "Timer.h"
#include "MsgpackRpc.h"
#include "RpcService.h"
#include "WrapTCPService.h"
#include "UsePacketSingleNetSession.h"
#include "../test/ClientExtOP.h"

WrapTcpService::PTR                         gTCPService;
brynet::TimerMgr::PTR                     gTimerMgr;
dodo::rpc::RpcService<dodo::rpc::MsgpackProtocol>   gRPC;

class SimulateClient : public UsePacketSingleNetSession
{
public:
    SimulateClient(){}

    ~SimulateClient()
    {
        if (mSendTimer.lock())
        {
            mSendTimer.lock()->cancel();
        }
    }

    virtual void    onEnter()
    {
        std::cout << "connect success" << std::endl;

        FixedPacket<1024 * 16> packet(static_cast<PACKET_OP_TYPE>(CLIENT_OP::CLIENT_OP_TEST));
        packet.writeBinary("test");
        sendPacket(packet.getData(), packet.getLen());
    }

    void    onPingReply(const string& value)
    {
        static int starttime = GetTickCount();
        static int count = 0;
        int nowtime = GetTickCount();
        count++;
        if ((nowtime - starttime) >= 1000)
        {
            cout << "recv ping reply " << count << "/s" << endl;
            count = 0;
            starttime = nowtime;
        }
        call("ping", "hello", [=](const string& value){
            onPingReply(value);
        });
    }


    virtual void    onClose()
    {
        cout << "on fake client dis connect" << endl;
    }

    template<typename... Args>
    void    call(const char* funname, const Args&... args)
    {
        sendLogicRpcString(gRPC.call(funname, args...));
    }

    template<typename... Args>
    void    callGameServer(const char* funname, const Args&... args)
    {
        sendGameServerRpcString(gRPC.call(funname, args...));
    }

    void            sendLogicRpcString(const string& value)
    {
        FixedPacket<1024 * 16> packet(1);
        packet.writeBinary(value);
        sendPacket(packet.getData(), packet.getLen());
    }
    void            sendGameServerRpcString(const string& value)
    {
        FixedPacket<1024 * 16> packet(1);
        packet.writeBinary(value);
        sendPacket(packet.getData(), packet.getLen());
    }
private:
    void            procPacket(PACKET_OP_TYPE op, const char* body, PACKET_LEN_TYPE bodyLen)
    {
        static int num = 0;
        static int starttime = GetTickCount();
        num++;

        if (op == static_cast<PACKET_OP_TYPE>(CLIENT_OP::CLIENT_OP_TEST))
        {
            FixedPacket<1024 * 16> packet(static_cast<PACKET_OP_TYPE>(CLIENT_OP::CLIENT_OP_TEST));
            packet.writeBinary("test");
            sendPacket(packet.getData(), packet.getLen());
        }

        if ((GetTickCount() - starttime) >= 1000)
        {
            cout << "recv base packet " << num << " /S" << endl;
            num = 0;
            starttime = GetTickCount();
        }
    }

    brynet::Timer::WeakPtr  mSendTimer;
};

int main()
{
    gTimerMgr = std::make_shared<brynet::TimerMgr>();
    gTCPService = std::make_shared<WrapTcpService>();
    gTCPService->startWorkThread(1, [](EventLoop::PTR){
        gTimerMgr->schedule();
    });

    /*Ä£Äâ¿Í»§¶Ë*/
    sock fd = ox_socket_connect(false, "127.0.0.1", 8001);
    if (SOCKET_ERROR != fd)
    {
        WrapAddNetSession(gTCPService, fd, make_shared<SimulateClient>(), 10000, 32 * 1024 * 1024);
    }
    else
    {
        std::cerr << "connect failed" << std::endl;
    }

    std::cin.get();
    return 0;
}