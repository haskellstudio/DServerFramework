#include "WrapTCPService.h"
#include "msgpackrpc.h"
#include "drpc.h"
#include "socketlibfunction.h"
#include "platform.h"
#include "packet.h"
#include "ConnectionServerRecvOP.h"
#include "ConnectionServerSendOP.h"
#include "CenterServerRecvOP.h"
#include "CenterServerSendOP.h"
#include "msgqueue.h"
#include "eventloop.h"
#include "NetThreadSession.h"
#include "ClientMirror.h"
#include "ConnectionServerConnection.h"
#include "CenterServerConnection.h"
#include "timer.h"
#include "SSDBMultiClient.h"
#include "spdlog/spdlog.h"
#include "ox_file.h"
#include "WrapLog.h"
#include "NetSession.h"
#include "UsePacketExtNetSession.h"
#include "etcdclient.h"
#include "WrapJsonValue.h"
#include "app_status.h"

WrapLog::PTR                                gDailyLogger;
TimerMgr::PTR                               gTimerMgr;
WrapServer::PTR                             gServer;
SSDBMultiClient::PTR                        gSSDBProxyClient;
struct lua_State* gLua = nullptr;

#include "lua_readtable.h"

#include "HelpFunction.h"
#include "AutoConnectionServer.h"
#include "CenterServerPassword.h"
#include "ConnectionServerPassword.h"
#include "google/protobuf/text_format.h"

string centerServerIP;
int centerServerPort;

extern void initLogicServerExt();

int main()
{
	srand(time(nullptr));

	string ssdbProxyIP;
	int ssdbProxyPort;
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
			aux_readluatable_byname(L, "SoloServerConfig", &config);
		}
		else
		{
			throw std::exception("not found lua file");
		}

		map<string, msvalue_s*>& _submapvalue = *config._map;

		ssdbProxyIP = map_at(_submapvalue, string("ssdbProxyIP"))->_str;
		ssdbProxyPort = atoi(map_at(_submapvalue, string("ssdbProxyPort"))->_str.c_str());
		centerServerIP = map_at(_submapvalue, string("centerServerIP"))->_str;
		centerServerPort = atoi(map_at(_submapvalue, string("centerServerPort"))->_str.c_str());
		gSelfID = atoi(map_at(_submapvalue, string("id"))->_str.c_str());
        /*TODO::读取配置*/
        gIsPrimary = true;
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

	gLua = luaL_newstate();
	luaopen_base(gLua);
	luaL_openlibs(gLua);

	gDailyLogger = std::make_shared<WrapLog>();
	gServer = std::make_shared<WrapServer>();
	gTimerMgr = std::make_shared<TimerMgr>();
	gSSDBProxyClient = std::make_shared<SSDBMultiClient>();
	gClientMirrorMgr = std::make_shared<ClientMirrorMgr>();
	
	ox_dir_create("logs");
	ox_dir_create("logs/SoloServer");
	gDailyLogger->setFile("", "logs/SoloServer/daily");

	gDailyLogger->info("server start!");

	EventLoop mainLoop;

	gSSDBProxyClient->startNetThread([&](){
		mainLoop.wakeup();
	});

	gDailyLogger->info("try connect ssdb proxy server: {}:{}", ssdbProxyIP, ssdbProxyPort);
    /*链接ssdb 路由服务器*/
	gSSDBProxyClient->asyncConnectionProxy(ssdbProxyIP.c_str(), ssdbProxyPort, 5000, true);

	gServer->startWorkThread(1, [&](EventLoop&)
	{
		syncNet2LogicMsgList(mainLoop);
	});

	{
		gDailyLogger->info("try connect center server: {}:{}", centerServerIP, centerServerPort);
		/*链接中心服务器*/
        startConnectThread<UsePacketExtNetSession, CenterServerConnection>(gDailyLogger, gServer, centerServerIP, centerServerPort);
	}

    initLogicServerExt();

    /*轮询etcd*/
    thread([etcdServers](){

        /*TODO::主线程的进程开启控制*/
        while (true)
        {
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
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));   /*  5s 轮询一次   */
        }
    }).detach();

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

		gSSDBProxyClient->pull();
		gSSDBProxyClient->forceSyncRequest();
	}

	return 0;
}