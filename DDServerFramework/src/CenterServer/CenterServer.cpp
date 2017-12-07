#include <fstream>
#include <sstream>
using namespace std;

#include <brynet/utils/app_status.h>
#include <brynet/net/WrapTCPService.h>
#include <brynet/utils/ox_file.h>
#include <brynet/timer/Timer.h>
#include <brynet/net/ListenThread.h>

#include "WrapLog.h"
#include "AutoConnectionServer.h"
#include "CenterServerSession.h"
#include "../../ServerConfig/ServerConfig.pb.h"
#include "google/protobuf/util/json_util.h"
#include "GlobalValue.h"

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
        logicServerListen->startListen(centerServerConfig.enableipv6(), centerServerConfig.bindip(), centerServerConfig.listenport(), [&](sock fd){
            WrapAddNetSession(gServer, fd, make_shared<UsePacketExtNetSession>(std::make_shared<CenterServerSession>()), 
                std::chrono::milliseconds(10000), 32 * 1024 * 1024);
        });

        gServer->startWorkThread(std::thread::hardware_concurrency(), [](EventLoop::PTR el){
            syncNet2LogicMsgList(gMainLoop);
        });

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

            procNet2LogicMsgList();
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