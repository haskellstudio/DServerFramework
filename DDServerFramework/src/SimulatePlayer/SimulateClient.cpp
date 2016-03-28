#include <set>
#include <string>
#include <iostream>

using namespace std;

#include "NetSession.h"
#include "WrapTCPService.h"
#include "socketlibfunction.h"
#include "platform.h"
#include "packet.h"
#include "timer.h"
#include "msgpackrpc.h"
#include "drpc.h"
#include "WrapTCPService.h"
#include "UsePacketSingleNetSession.h"
#include "../test/ClientExtOP.h"

WrapServer::PTR                         gTCPService;
TimerMgr::PTR                           gTimerMgr;
dodo::rpc<dodo::MsgpackProtocol>        gRPC;

class SimulateClient : public UsePacketSingleNetSession
{
public:
    SimulateClient(){}

    ~SimulateClient()
    {
        if (mSendTimer.lock())
        {
            mSendTimer.lock()->Cancel();
        }
    }

    virtual void    onEnter()
    {
        FixedPacket<1024 * 16> packet(CLIENT_OP_TEST);
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

        if (op == CLIENT_OP_TEST)
        {
            FixedPacket<1024 * 16> packet(CLIENT_OP_TEST);
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

    Timer::WeakPtr  mSendTimer;
};

int main()
{
    gTimerMgr = std::make_shared<TimerMgr>();
    gTCPService = std::make_shared<WrapServer>();
    gTCPService->startWorkThread(1, [](EventLoop&){
        gTimerMgr->Schedule();
    });

    /*Ä£Äâ¿Í»§¶Ë*/
    sock fd = ox_socket_connect(false, "127.0.0.1", 29071);
    WrapAddNetSession(gTCPService, fd, make_shared<SimulateClient>(), 10000, 32*1024*1024);
    std::cin.get();
    return 0;
}