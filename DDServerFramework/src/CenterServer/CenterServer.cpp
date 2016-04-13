#include "vld.h"

#include "app_status.h"
#include "WrapTCPService.h"
#include "ox_file.h"
#include "WrapLog.h"
#include "timer.h"
#include "SSDBMultiClient.h"
#include "lua_readtable.h"
#include "HelpFunction.h"
#include "AutoConnectionServer.h"
#include "CenterServerPassword.h"
#include "CenterServerSession.h"

WrapLog::PTR            gDailyLogger;
SSDBMultiClient::PTR    gSSDBClient;
WrapServer::PTR         gServer;
TimerMgr::PTR           gLogicTimerMgr;

extern void initCenterServerExt();

int main()
{
    int listenPort;
    string ssdbServerIP;
    int ssdbServerPort;
    
    try
    {
        struct msvalue_s config(true);
        struct lua_State* L = luaL_newstate();
        luaopen_base(L);
        luaL_openlibs(L);

        CenterServerPassword::getInstance().load(L);

        if (lua_tinker::dofile(L, "ServerConfig//CenterServerConfig.lua"))
        {
            aux_readluatable_byname(L, "CenterServerConfig", &config);
        }
        else
        {
            throw std::runtime_error("not found ServerConfig//CenterServerConfig.lua file");
        }

        map<string, msvalue_s*>& _submapvalue = *config._map;

        listenPort = atoi(map_at(_submapvalue, string("listenPort"))->_str.c_str());
        ssdbServerIP = map_at(_submapvalue, string("ssdbServerIP"))->_str;
        ssdbServerPort = atoi(map_at(_submapvalue, string("ssdbServerPort"))->_str.c_str());
        lua_close(L);
        L = nullptr;
    }
    catch (const std::exception& e)
    {
        errorExit(e.what());
    }

    gLogicTimerMgr = std::make_shared<TimerMgr>();
    gDailyLogger = std::make_shared<WrapLog>();
    gServer = std::make_shared<WrapServer>();
    gSSDBClient = std::make_shared<SSDBMultiClient>();
    CenterServerSessionGlobalData::init();

    spdlog::set_level(spdlog::level::info);

    ox_dir_create("logs");
    ox_dir_create("logs/CenterServer");
    gDailyLogger->setFile("", "logs/CenterServer/daily");

    {
        EventLoop mainLoop;

        gSSDBClient->startNetThread([&](){
            mainLoop.wakeup();
        });

        gDailyLogger->info("try connect ssdb server:{}:{}", ssdbServerIP, ssdbServerPort);
        gSSDBClient->asyncConnection(ssdbServerIP.c_str(), ssdbServerPort, 5000, true);

        /*开启内部监听服务器，处理逻辑服务器的链接*/
        gDailyLogger->info("listen logic server port:{}", listenPort);
        ListenThread    logicServerListen;
        logicServerListen.startListen(false, "127.0.0.1", listenPort, nullptr, nullptr, [&](int fd){
            WrapAddNetSession(gServer, fd, make_shared<UsePacketExtNetSession>(std::make_shared<CenterServerSession>()), 10000, 32 * 1024 * 1024);
        });

        gServer->startWorkThread(ox_getcpunum(), [&](EventLoop& el){
            syncNet2LogicMsgList(mainLoop);
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

            mainLoop.loop(gLogicTimerMgr->IsEmpty() ? 1 : gLogicTimerMgr->NearEndMs());

            gLogicTimerMgr->Schedule();

            procNet2LogicMsgList();

            gSSDBClient->pull();
            gSSDBClient->forceSyncRequest();
        }

        logicServerListen.closeListenThread();
    }

    gServer->getService()->stopWorkerThread();
    gSSDBClient->stopService();
    gDailyLogger->stop();

    gDailyLogger = nullptr;
    gSSDBClient = nullptr;
    gServer = nullptr;
    gLogicTimerMgr = nullptr;
    CenterServerSessionGlobalData::destroy();

    VLDReportLeaks();

    return 0;
}