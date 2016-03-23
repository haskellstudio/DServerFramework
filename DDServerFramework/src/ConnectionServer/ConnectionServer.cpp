#include <set>

#include "WrapTCPService.h"
#include "timer.h"
#include "ox_file.h"
#include "WrapLog.h"
#include "lua_readtable.h"
#include "HelpFunction.h"
#include "ConnectionServerPassword.h"
#include "AutoConnectionServer.h"
#include "etcdclient.h"
#include "WrapJsonValue.h"
#include "app_status.h"
#include "ClientSession.h"
#include "LogicServerSession.h"

/*  所有的primary server和slave server链接(key为内部游戏服务器运行时所分配的逻辑ID   */
unordered_map<int, BaseNetSession::PTR>     gAllPrimaryServers;
unordered_map<int, BaseNetSession::PTR>     gAllSlaveServers;

WrapServer::PTR                         gServer;
ListenThread::PTR                       gListenClient;
ListenThread::PTR                       gListenLogic;
WrapLog::PTR                            gDailyLogger;

string selfIP;
int gSelfID = 0;
int portForClient;
int portForLogicServer;

/*
    链接服务器（在服务器架构中存在N个）：
    1：将其地址告诉etcd集群. (后期的均衡服务器可以访问etcd来获取所有的链接服务器地址，并按照一定机制分配给客户端)。
    2：负责处理客户端和内部逻辑服务器的链接，转发客户端和内部服务器之间的通信
*/

static std::vector<std::tuple<string, int>> etcdServers;

static bool readConfig()
{
    bool ret = false;

    try
    {
        struct msvalue_s config(true);
        struct lua_State* L = luaL_newstate();
        luaopen_base(L);
        luaL_openlibs(L);
        /*TODO::由启动参数指定配置路径*/
        ConnectionServerPassword::getInstance().load(L);
        if (lua_tinker::dofile(L, "ServerConfig//ConnectionServerConfig.lua"))
        {
            aux_readluatable_byname(L, "ConnectionServerConfig", &config);
        }
        else
        {
            throw std::runtime_error("not found ServerConfig//ConnectionServerConfig.lua file");
        }

        map<string, msvalue_s*>& _submapvalue = *config._map;

        selfIP = map_at(_submapvalue, string("selfIP"))->_str;
        portForClient = atoi(map_at(_submapvalue, string("portForClient"))->_str.c_str());
        portForLogicServer = atoi(map_at(_submapvalue, string("portForLogicServer"))->_str.c_str());
        gSelfID = atoi(map_at(_submapvalue, string("id"))->_str.c_str());
        map<string, msvalue_s*>& etcdConfig = *map_at(_submapvalue, string("etcdservers"))->_map;
        for (auto& v : etcdConfig)
        {
            map<string, msvalue_s*>& oneconfig = *((v.second)->_map);
            etcdServers.push_back(std::make_tuple(map_at(oneconfig, string("ip"))->_str, atoi(map_at(oneconfig, string("port"))->_str.c_str())));
        }
        lua_close(L);
        L = nullptr;
        ret = true;
    }
    catch (const std::exception& e)
    {
        errorExit(e.what());
    }

    return ret;
}

static void startServer()
{
    gServer = std::make_shared<WrapServer>();
    gServer->startWorkThread(ox_getcpunum(), [](EventLoop&){
    });

    /*开启对外客户端端口*/
    gListenClient = std::make_shared<ListenThread>();
    gListenClient->startListen(portForClient, nullptr, nullptr, [&](int fd){
        WrapAddNetSession(gServer, fd, std::make_shared<ConnectionClientSession>(), -1, 32 * 1024);
    });

    /*开启对内逻辑服务器端口*/
    gListenLogic = std::make_shared<ListenThread>();
    gListenLogic->startListen(portForLogicServer, nullptr, nullptr, [&](int fd){
        WrapAddNetSession(gServer, fd, std::make_shared<LogicServerSession>(), 10000, 32 * 1024 * 1024);
    });
}

int main()
{
    if (readConfig())
    {
        ox_dir_create("logs");
        ox_dir_create("logs/ConnectionServer");

        gDailyLogger = std::make_shared<WrapLog>();
        gDailyLogger->setFile("", "logs/ConnectionServer/daily");

        startServer();

        std::map<string, string> etcdKV;

        WrapJsonValue serverJson(rapidjson::kObjectType);
        serverJson.AddMember("ID", gSelfID);
        serverJson.AddMember("IP", selfIP);
        serverJson.AddMember("portForClient", portForClient);
        serverJson.AddMember("portForLogicServer", portForLogicServer);

        etcdKV["value"] = serverJson.toString();
        etcdKV["ttl"] = std::to_string(15); /*存活时间为15秒*/

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

            for (auto& etcd : etcdServers)
            {
                if (!etcdSet(std::get<0>(etcd), std::get<1>(etcd), string("ConnectionServerList/") + std::to_string(gSelfID), etcdKV, 5000).getBody().empty())
                {
                    break;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5000));   /*5s 重设一次*/
        }

        gListenClient->closeListenThread();
        gListenLogic->closeListenThread();
        gServer->getService()->closeListenThread();
        gServer->getService()->stopWorkerThread();
        gDailyLogger->stop();
    }

    return 0;
}