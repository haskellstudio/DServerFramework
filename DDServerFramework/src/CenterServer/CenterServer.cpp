#include <fstream>
#include <sstream>
using namespace std;

#include <brynet/utils/app_status.h>
#include <brynet/net/WrapTCPService.h>
#include <brynet/utils/ox_file.h>
#include <brynet/timer/Timer.h>
#include <brynet/net/ListenThread.h>

#include "WrapLog.h"
#include "CenterServerSession.h"
#include "../../ServerConfig/ServerConfig.pb.h"
#include "google/protobuf/util/json_util.h"
#include "GlobalValue.h"
#include "../Common/SocketSession.h"

using namespace brynet::net;

ServerConfig::CenterServerConfig centerServerConfig;

extern void initCenterServerExt();

int main()
{
    ifstream  centerServerConfigFile("ServerConfig/CenterServerConfig.json");
    std::stringstream buffer;
    buffer << centerServerConfigFile.rdbuf();

    google::protobuf::util::Status s = google::protobuf::util::JsonStringToMessage(buffer.str(), &centerServerConfig);
    if (!s.ok())
    {
        std::cerr << "load config error:" << s.error_message() << std::endl;
        exit(-1);
    }

    gLogicTimerMgr = std::make_shared<brynet::TimerMgr>();
    gDailyLogger = std::make_shared<WrapLog>();
    gServer = std::make_shared<brynet::net::WrapTcpService>();
    CenterServerSessionGlobalData::init();

    spdlog::set_level(spdlog::level::info);

    ox_dir_create("logs");
    ox_dir_create("logs/CenterServer");
    gDailyLogger->setFile("", "logs/CenterServer/daily");

    {
        gMainLoop = std::make_shared<EventLoop>();
        /*开启内部监听服务器，处理逻辑服务器的链接*/
        gDailyLogger->info("listen logic server port:{}", centerServerConfig.bindip());
        auto logicServerListen = ListenThread::Create();
        logicServerListen->startListen(centerServerConfig.enableipv6(), centerServerConfig.bindip(), centerServerConfig.listenport(), [=](sock fd){
            gServer->addSession(fd, [](const TCPSession::PTR& session) {
                // 构造逻辑对象
                auto ss = std::make_shared<SocketSession>();

                auto sendSession = std::make_shared<SocketSession>();
                sendSession->childHandler([session](const Context&, const std::string& buffer, const SocketSession::INTERCEPTOR& next)  {
                    session->send(buffer.c_str(), buffer.size());
                });
                auto logicSession = std::make_shared<CenterServerSession>(session->getIP(), sendSession);

                gMainLoop->pushAsyncProc([logicSession]() {
                    logicSession->onEnter();
                });

                session->setHeartBeat(std::chrono::nanoseconds::zero());

                session->setDisConnectCallback([logicSession](const TCPSession::PTR& session) {
                    gMainLoop->pushAsyncProc([logicSession]() {
                        logicSession->onClose();
                    });
                });

                // TODO::将解包也作为handle的一个拦截器,不过需要修改网络库的消息接收回调(因为目前的回调需要返回值)
                session->setDataCallback([ss](const TCPSession::PTR& session, const char* buffer, size_t len) {
                    const char* parse_str = buffer;
                    size_t total_proc_len = 0;
                    PACKET_LEN_TYPE left_len = static_cast<PACKET_LEN_TYPE>(len);

                    while (true)
                    {
                        bool flag = false;
                        if (left_len >= PACKET_HEAD_LEN)
                        {
                            ReadPacket rp(parse_str, left_len);

                            PACKET_LEN_TYPE packet_len = rp.readPacketLen();
                            if (left_len >= packet_len && packet_len >= PACKET_HEAD_LEN)
                            {
                                PACKET_OP_TYPE op = rp.readOP();

                                ss->handle(std::string(parse_str, packet_len));
                                total_proc_len += packet_len;
                                parse_str += packet_len;
                                left_len -= packet_len;
                                flag = true;
                            }
                            rp.skipAll();
                        }

                        if (!flag)
                        {
                            break;
                        }
                    }

                    return total_proc_len;
                });

                ss->childHandler([logicSession](Context& context, const std::string& buffer, const SocketSession::INTERCEPTOR& next) {
                    gMainLoop->pushAsyncProc([logicSession, buffer]() {
                        logicSession->onMsg(buffer.c_str(), buffer.size());
                    });
                    next(context, buffer);
                });
            }, false, nullptr, 32 * 1024);
        });

        gServer->startWorkThread(std::thread::hardware_concurrency());

        gDailyLogger->info("server start!");

        initCenterServerExt();

        while (true)
        {
            if (app_kbhit())
            {
                string input;
                std::getline(std::cin, input);
                gDailyLogger->warn("console input {}", input);

                if (input == "quit")
                {
                    break;
                }
            }

            auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(gLogicTimerMgr->nearLeftTime());
            gMainLoop->loop(gLogicTimerMgr->isEmpty() ? 1 : tmp.count());

            gLogicTimerMgr->schedule();
        }

        logicServerListen->stopListen();
    }

    gServer->getService()->stopWorkerThread();
    gDailyLogger->stop();

    gDailyLogger = nullptr;
    gServer = nullptr;
    gLogicTimerMgr = nullptr;
    CenterServerSessionGlobalData::destroy();

    return 0;
}