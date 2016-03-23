#include "WrapTCPService.h"
#include "timer.h"
#include "SSDBMultiClient.h"
#include "ox_file.h"
#include "WrapLog.h"
#include "etcdclient.h"
#include "WrapJsonValue.h"
#include "app_status.h"
#include "lua_readtable.h"

#include "HelpFunction.h"
#include "AutoConnectionServer.h"
#include "CenterServerPassword.h"
#include "ConnectionServerPassword.h"

#include "ClientMirror.h"
#include "ConnectionServerConnection.h"
#include "CenterServerConnection.h"

WrapLog::PTR                                gDailyLogger;
TimerMgr::PTR                               gTimerMgr;
WrapServer::PTR                             gServer;
SSDBMultiClient::PTR                        gSSDBClient;

extern void initLogicServerExt();

int main()
{
    srand(time(nullptr));

    string ssdbServerIP;
    int ssdbServerPort;
    std::vector<std::tuple<string, int>> etcdServers;

    try
    {
        struct msvalue_s config(true);
        struct lua_State* L = luaL_newstate();
        luaopen_base(L);
        luaL_openlibs(L);
        /*TODO::由启动参数指定配置路径*/

        ConnectionServerPassword::getInstance().load(L);
        CenterServerPassword::getInstance().load(L);

        if (lua_tinker::dofile(L, "ServerConfig//LogicServerConfig.lua"))
        {
            aux_readluatable_byname(L, "LogicServerConfig", &config);
        }
        else
        {
            throw std::exception("not found lua file");
        }

        map<string, msvalue_s*>& _submapvalue = *config._map;

        ssdbServerIP = map_at(_submapvalue, string("ssdbServerIP"))->_str;
        ssdbServerPort = atoi(map_at(_submapvalue, string("ssdbServerPort"))->_str.c_str());
        gCenterServerIP = map_at(_submapvalue, string("centerServerIP"))->_str;
        gCenterServerPort = atoi(map_at(_submapvalue, string("centerServerPort"))->_str.c_str());
        gSelfID = atoi(map_at(_submapvalue, string("id"))->_str.c_str());
        gIsPrimary = atoi(map_at(_submapvalue, string("isPrimary"))->_str.c_str()) == 1;
        map<string, msvalue_s*>& etcdConfig = *map_at(_submapvalue, string("etcdservers"))->_map;
        for (auto& v : etcdConfig)
        {
            map<string, msvalue_s*>& oneconfig = *((v.second)->_map);
            etcdServers.push_back(std::make_tuple(map_at(oneconfig, string("ip"))->_str, atoi(map_at(oneconfig, string("port"))->_str.c_str())));
        }

        lua_close(L);
        L = nullptr;
    }
    catch (const std::exception& e)
    {
        errorExit(e.what());
    }

    gDailyLogger = std::make_shared<WrapLog>();
    gServer = std::make_shared<WrapServer>();
    gTimerMgr = std::make_shared<TimerMgr>();
    gSSDBClient = std::make_shared<SSDBMultiClient>();
    gClientMirrorMgr = std::make_shared<ClientMirrorMgr>();
    
    ox_dir_create("logs");
    ox_dir_create("logs/LogicServer");
    std::string logDir = std::string("logs/LogicServer/") + std::to_string(gSelfID);
    ox_dir_create(logDir.c_str());
    gDailyLogger->setFile("", (logDir + "/daily").c_str());

    gDailyLogger->info("server start!");

    EventLoop mainLoop;

    gSSDBClient->startNetThread([&](){
        mainLoop.wakeup();
    });

    gDailyLogger->info("try connect ssdb server: {}:{}", ssdbServerIP, ssdbServerPort);
    /*链接ssdb 路由服务器*/
    gSSDBClient->asyncConnection(ssdbServerIP.c_str(), ssdbServerPort, 5000, true);

    gServer->startWorkThread(ox_getcpunum(), [&](EventLoop&)
    {
        syncNet2LogicMsgList(mainLoop);
    });

    {
        gDailyLogger->info("try connect center server: {}:{}", gCenterServerIP, gCenterServerPort);
        /*链接中心服务器*/
        startConnectThread<UsePacketExtNetSession, CenterServerConnection>(gDailyLogger, gServer, gCenterServerIP, gCenterServerPort);
    }

    initLogicServerExt();

    /*轮询etcd*/
    bool etcdPullIsClose = false;

    std::thread etcdPullThread = std::thread([etcdServers, &etcdPullIsClose](){

        while (!etcdPullIsClose)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2500));
            for (auto& etcd : etcdServers)
            {
                HTTPParser result = etcdGet(std::get<0>(etcd), std::get<1>(etcd), "ConnectionServerList", 5000);
                if (!result.getBody().empty())
                {
                    unordered_map<int32_t, std::tuple<string, int>> currentServer;

                    gDailyLogger->info("list server json:{}", result.getBody());
                    rapidjson::Document doc;
                    doc.Parse<0>(result.getBody().c_str());
                    auto it = doc.FindMember("node");
                    if (it != doc.MemberEnd())
                    {
                        it = doc.FindMember("node")->value.FindMember("nodes");
                        if (it != doc.FindMember("node")->value.MemberEnd())
                        {
                            rapidjson::Value& nodes = doc.FindMember("node")->value.FindMember("nodes")->value;
                            for (size_t i = 0; i < nodes.Size(); i++)
                            {
                                rapidjson::Value& oneServer = nodes[i].FindMember("value")->value;
                                rapidjson::Document oneServerJson;
                                oneServerJson.Parse<0>(oneServer.GetString());
                                int id = oneServerJson.FindMember("ID")->value.GetInt();
                                string ip = oneServerJson.FindMember("IP")->value.GetString();
                                int port = oneServerJson.FindMember("portForLogicServer")->value.GetInt();

                                currentServer[id] = std::make_tuple(ip, port);
                            }
                        }
                    }

                    tryCompareConnect(currentServer);
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        }
    });

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

        mainLoop.loop(gTimerMgr->IsEmpty() ? 1 : gTimerMgr->NearEndMs());

        gTimerMgr->Schedule();

        procNet2LogicMsgList();

        gSSDBClient->pull();
        gSSDBClient->forceSyncRequest();
    }

    gDailyLogger->stop();
    gServer->getService()->stopWorkerThread();
    gSSDBClient->stopService();

    etcdPullIsClose = true;
    if (etcdPullThread.joinable())
    {
        etcdPullThread.join();
    }

    return 0;
}