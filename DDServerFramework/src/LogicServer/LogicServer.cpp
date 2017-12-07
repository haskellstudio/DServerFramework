#include <fstream>
#include <iostream>
#include <sstream>

#include <brynet/net/WrapTCPService.h>
#include <brynet/timer/Timer.h>
#include <brynet/utils/ox_file.h>
#include <brynet/utils/app_status.h>

#include "../Common/etcdclient.h"
#include "WrapLog.h"
#include "WrapJsonValue.h"

#include "HelpFunction.h"
#include "AutoConnectionServer.h"

#include "ClientMirror.h"
#include "ConnectionServerConnection.h"
#include "google/protobuf/util/json_util.h"
#include "GlobalValue.h"

extern void initLogicServerExt();

int main()
{
    srand(time(nullptr));

    std::stringstream buffer;
    std::ifstream  logicServerConfigFile("ServerConfig/LogicServerConfig.json");
    buffer << logicServerConfigFile.rdbuf();

    google::protobuf::util::Status status = google::protobuf::util::JsonStringToMessage(buffer.str(), &logicServerConfig);
    if (!status.ok())
    {
        std::cerr << "load config error:" << status.error_message() << std::endl;
        exit(-1);
    }
    buffer.str("");

    std::ifstream  centerServerConfigFile("ServerConfig/CenterServerConfig.json");
    buffer << centerServerConfigFile.rdbuf();

    status = google::protobuf::util::JsonStringToMessage(buffer.str(), &centerServerConfig);
    if (!status.ok())
    {
        std::cerr << "load config error:" << status.error_message() << std::endl;
        exit(-1);
    }

    gDailyLogger = std::make_shared<WrapLog>();
    gServer = std::make_shared<WrapTcpService>();
    gLogicTimerMgr = std::make_shared<brynet::TimerMgr>();
    gClientMirrorMgr = std::make_shared<ClientMirrorMgr>();
    
    ox_dir_create("logs");
    ox_dir_create("logs/LogicServer");
    std::string logDir = std::string("logs/LogicServer/") + std::to_string(logicServerConfig.id());
    ox_dir_create(logDir.c_str());
    gDailyLogger->setFile("", (logDir + "/daily").c_str());

    gDailyLogger->info("server start!");

    gMainLoop = std::make_shared<EventLoop>();

    gServer->startWorkThread(std::thread::hardware_concurrency(), [](EventLoop::PTR)
    {
        syncNet2LogicMsgList(gMainLoop);
    });

    {
        gDailyLogger->info("try connect center server: {}:{}", centerServerConfig.bindip(), centerServerConfig.listenport());
        /*链接中心服务器*/
        startConnectThread<UsePacketExtNetSession, CenterServerConnection>(gDailyLogger,
            gServer, 
            centerServerConfig.enableipv6(), 
            centerServerConfig.bindip(), 
            centerServerConfig.listenport());
    }

    initLogicServerExt();

    {
        std::unordered_map<int32_t, std::tuple<std::string, int, std::string>> testConnectionServer;
        testConnectionServer[0] = std::tuple<std::string, int, std::string>("127.0.0.1", 5777, "hello");
        tryCompareConnect(testConnectionServer);
    }

    /*轮询etcd*/
    bool etcdPullIsClose = false;

    std::thread etcdPullThread = std::thread([&etcdPullIsClose](){

        while (!etcdPullIsClose)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2500));
            for (auto& etcd : logicServerConfig.etcdservers())
            {
                HTTPParser result = etcdGet(etcd.ip(), etcd.port(), "ConnectionServerList", 5000);
                if (result.getBody().empty())
                {
                    continue;
                }

                std::unordered_map<int32_t, std::tuple<std::string, int, std::string>> currentServer;

                gDailyLogger->info("list server json:{}", result.getBody());
                rapidjson::Document doc;
                doc.Parse<0>(result.getBody().c_str());
                auto it = doc.FindMember("node");
                if (it == doc.MemberEnd())
                {
                    continue;
                }

                it = doc.FindMember("node")->value.FindMember("nodes");
                if (it == doc.FindMember("node")->value.MemberEnd())
                {
                    continue;
                }

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

                tryCompareConnect(currentServer);
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        }
    });

    while (true)
    {
        if (app_kbhit())
        {
            std::string input;
            std::getline(std::cin, input);
            gDailyLogger->warn("console input {}", input);

            if (input == "quit")
            {
                break;
            }
        }

        auto tmp = std::chrono::duration_cast<std::chrono::microseconds>(gLogicTimerMgr->nearLeftTime());
        gMainLoop->loop(gLogicTimerMgr->isEmpty() ? 1 : tmp.count());

        gLogicTimerMgr->schedule();

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