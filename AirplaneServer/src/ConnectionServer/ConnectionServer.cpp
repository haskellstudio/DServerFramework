#include <set>

#include "WrapTCPService.h"
#include "NetSession.h"
#include "socketlibfunction.h"
#include "platform.h"
#include "packet.h"
#include "timer.h"
#include "ox_file.h"
#include "WrapLog.h"
#include "ClientSession.h"
#include "LogicServerSession.h"
#include "lua_readtable.h"
#include "HelpFunction.h"
#include "ConnectionServerPassword.h"
#include "AutoConnectionServer.h"
#include "etcdclient.h"
#include "WrapJsonValue.h"
#include "app_status.h"

WrapLog::PTR gDailyLogger;

/*  所有的primary server和slave server链接(key为内部游戏服务器运行时所分配的逻辑ID   */
unordered_map<int, BaseNetSession::PTR>     gAllPrimaryServers;
unordered_map<int, BaseNetSession::PTR>     gAllSlaveServers;

WrapServer::PTR                         gServer;
TimerMgr::PTR                           gTimerMgr;

string selfIP;
int gSelfID = 0;
int portForClient;
int portForLogicServer;

/*
    链接服务器（在服务器架构中存在N个）：
    1：将其地址告诉etcd集群. (后期的均衡服务器可以访问etcd来获取所有的链接服务器地址，并按照一定机制分配给玩家)。
    2：负责处理玩家和内部逻辑服务器的链接，转发玩家和内部服务器之间的通信
*/

int main()
{
    std::vector<std::tuple<string, int>> etcdServers;

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
    }
    catch (const std::exception& e)
    {
        errorExit(e.what());
    }

    gDailyLogger = std::make_shared<WrapLog>();
    gTimerMgr = std::make_shared<TimerMgr>();

    gServer = std::make_shared<WrapServer>();
    /*  这里线程数为1， gTimerMgr 安全，且还有全局逻辑链接管理，1个线程是安全的*/
    gServer->startWorkThread(1, [](EventLoop&){
        gTimerMgr->Schedule();
    });

    ox_dir_create("logs");
    ox_dir_create("logs/ConnectionServer");
    gDailyLogger->setFile("", "logs/ConnectionServer/daily");

    gDailyLogger->info("start port:{} for client", portForClient);
    /*开启对外客户端端口*/
    ListenThread    clientListen;
    clientListen.startListen(portForClient, nullptr, nullptr, [&](int fd){
        WrapAddNetSession(gServer, fd, std::make_shared<ConnectionClientSession>(), -1, 32*1024);
    });

    gDailyLogger->info("start port:{} for logic server", portForLogicServer);
    /*开启对内逻辑服务器端口*/
    ListenThread logicServerListen;
    logicServerListen.startListen(portForLogicServer, nullptr, nullptr, [&](int fd){
        WrapAddNetSession(gServer, fd, std::make_shared<LogicServerSession>(), 10000, 32*1024*1024);
    });

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

    return 0;
}