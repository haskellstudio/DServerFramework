#include <fstream>
#include <sstream>
using namespace std;

#include "app_status.h"
#include "WrapTCPService.h"
#include "ox_file.h"
#include "WrapLog.h"
#include "Timer.h"
#include "AutoConnectionServer.h"
#include "CenterServerSession.h"
#include "../../ServerConfig/ServerConfig.pb.h"
#include "google/protobuf/util/json_util.h"
#include "GlobalValue.h"

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
        logicServerListen->startListen(centerServerConfig.enableipv6(), centerServerConfig.bindip(), centerServerConfig.listenport(), nullptr, nullptr, [&](sock fd){
            WrapAddNetSession(gServer, fd, make_shared<UsePacketExtNetSession>(std::make_shared<CenterServerSession>()), 10000, 32 * 1024 * 1024);
        });

        gServer->startWorkThread(ox_getcpunum(), [](EventLoop::PTR el){
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

            gMainLoop->loop(gLogicTimerMgr->isEmpty() ? 1 : gLogicTimerMgr->nearEndMs());

            gLogicTimerMgr->schedule();

            procNet2LogicMsgList();
        }

        logicServerListen->closeListenThread();
    }

    gServer->getService()->stopWorkerThread();
    gDailyLogger->stop();

    gDailyLogger = nullptr;
    gServer = nullptr;
    gLogicTimerMgr = nullptr;
    CenterServerSessionGlobalData::destroy();

    return 0;
}