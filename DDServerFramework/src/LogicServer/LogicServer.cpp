#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

#include "WrapTCPService.h"
#include "Timer.h"
#include "ox_file.h"
#include "WrapLog.h"
#include "etcdclient.h"
#include "WrapJsonValue.h"
#include "app_status.h"
#include "lua_readtable.h"

#include "HelpFunction.h"
#include "AutoConnectionServer.h"

#include "ClientMirror.h"
#include "ConnectionServerConnection.h"
#include "CenterServerConnection.h"
#include "google/protobuf/util/json_util.h"
#include "../../ServerConfig/ServerConfig.pb.h"

ServerConfig::CenterServerConfig centerServerConfig;
ServerConfig::LogicServerConfig logicServerConfig;

WrapLog::PTR                                gDailyLogger;
dodo::TimerMgr::PTR                         gTimerMgr;
WrapServer::PTR                             gServer;
shared_ptr<EventLoop> mainLoop;

extern void initLogicServerExt();

int main()
{
    srand(time(nullptr));

    std::stringstream buffer;
    ifstream  logicServerConfigFile("ServerConfig/LogicServerConfig.json");
    buffer << logicServerConfigFile.rdbuf();

    google::protobuf::util::Status status = google::protobuf::util::JsonStringToMessage(buffer.str(), &logicServerConfig);
    if (!status.ok())
    {
        std::cerr << "load config error:" << status.error_message() << std::endl;
        exit(-1);
    }
    buffer.str("");

    ifstream  centerServerConfigFile("ServerConfig/CenterServerConfig.json");
    buffer << centerServerConfigFile.rdbuf();

    status = google::protobuf::util::JsonStringToMessage(buffer.str(), &centerServerConfig);
    if (!status.ok())
    {
        std::cerr << "load config error:" << status.error_message() << std::endl;
        exit(-1);
    }

    gDailyLogger = std::make_shared<WrapLog>();
    gServer = std::make_shared<WrapServer>();
    gTimerMgr = std::make_shared<dodo::TimerMgr>();
    gClientMirrorMgr = std::make_shared<ClientMirrorMgr>();
    
    ox_dir_create("logs");
    ox_dir_create("logs/LogicServer");
    std::string logDir = std::string("logs/LogicServer/") + std::to_string(logicServerConfig.id());
    ox_dir_create(logDir.c_str());
    gDailyLogger->setFile("", (logDir + "/daily").c_str());

    gDailyLogger->info("server start!");

    mainLoop = std::make_shared<EventLoop>();

    gServer->startWorkThread(ox_getcpunum(), [](EventLoop&)
    {
        syncNet2LogicMsgList(mainLoop);
    });

    {
        gDailyLogger->info("try connect center server: {}:{}", centerServerConfig.bindip(), centerServerConfig.listenport());
        /*链接中心服务器*/
        startConnectThread<UsePacketExtNetSession, CenterServerConnection>(gDailyLogger, gServer, centerServerConfig.enableipv6(), centerServerConfig.bindip(), centerServerConfig.listenport());
    }

    initLogicServerExt();

    /*轮询etcd*/
    bool etcdPullIsClose = false;

    std::thread etcdPullThread = std::thread([&etcdPullIsClose](){

        while (!etcdPullIsClose)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2500));
            for (auto& etcd : logicServerConfig.etcdservers())
            {
                HTTPParser result = etcdGet(etcd.ip(), etcd.port(), "ConnectionServerList", 5000);
                if (!result.getBody().empty())
                {
                    unordered_map<int32_t, std::tuple<string, int, string>> currentServer;

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
                            for (rapidjson::SizeType i = 0; i < nodes.Size(); i++)
                            {
                                rapidjson::Value& oneServer = nodes[i].FindMember("value")->value;
                                ServerConfig::ConnectionServerConfig oneConnectionServerConfig;
                                if (google::protobuf::util::JsonStringToMessage(oneServer.GetString(), &oneConnectionServerConfig).ok())
                                {
                                    currentServer[oneConnectionServerConfig.id()] = std::make_tuple(oneConnectionServerConfig.bindip(), oneConnectionServerConfig.portforlogicserver(), oneConnectionServerConfig.logicserverloginpassword());
                                }
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

        mainLoop->loop(gTimerMgr->isEmpty() ? 1 : gTimerMgr->nearEndMs());

        gTimerMgr->schedule();

        procNet2LogicMsgList();
    }

    gDailyLogger->stop();
    gServer->getService()->stopWorkerThread();

    etcdPullIsClose = true;
    if (etcdPullThread.joinable())
    {
        etcdPullThread.join();
    }

    return 0;
}