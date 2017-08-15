#include <set>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

#include "WrapTCPService.h"
#include "Timer.h"
#include "ox_file.h"
#include "WrapLog.h"
#include "AutoConnectionServer.h"
#include "etcdclient.h"
#include "WrapJsonValue.h"
#include "app_status.h"
#include "ClientSession.h"
#include "LogicServerSession.h"
#include "../../ServerConfig/ServerConfig.pb.h"
#include "google/protobuf/util/json_util.h"

WrapLog::PTR                            gDailyLogger;

/*
    链接服务器（在服务器架构中存在N个）：
    1：将其地址告诉etcd集群. (后期的均衡服务器可以访问etcd来获取所有的链接服务器地址，并按照一定机制分配给客户端)。
    2：负责处理客户端和内部逻辑服务器的链接，转发客户端和内部服务器之间的通信
*/

int main()
{
    std::ifstream  connetionServerConfigFile("ServerConfig/ConnetionServerConfig.json");
    std::stringstream buffer;
    buffer << connetionServerConfigFile.rdbuf();

    ServerConfig::ConnectionServerConfig connectionServerConfig;
    google::protobuf::util::Status s = google::protobuf::util::JsonStringToMessage(buffer.str(), &connectionServerConfig);
    if (!s.ok())
    {
        std::cerr << "load config error:" << s.error_message() << std::endl;
        exit(-1);
    }

    ox_dir_create("logs");
    ox_dir_create("logs/ConnectionServer");

    gDailyLogger = std::make_shared<WrapLog>();
    gDailyLogger->setFile("", "logs/ConnectionServer/daily");

    auto service = std::make_shared<WrapTcpService>();
    
    service->startWorkThread(ox_getcpunum(), [](EventLoop::PTR) {
    });

    /*开启对外客户端端口*/
    auto clientListener = ListenThread::Create();
    clientListener->startListen(connectionServerConfig.enableipv6(), connectionServerConfig.bindip(), connectionServerConfig.portforclient(), nullptr, nullptr, [=](sock fd) {
        WrapAddNetSession(service, fd, std::make_shared<ConnectionClientSession>(connectionServerConfig.id()), -1, 32 * 1024);
    });

    /*开启对内逻辑服务器端口*/
    auto logicServerListener = ListenThread::Create();
    logicServerListener->startListen(connectionServerConfig.enableipv6(), connectionServerConfig.bindip(), connectionServerConfig.portforlogicserver(), nullptr, nullptr, [=](sock fd) {
        WrapAddNetSession(service, fd, std::make_shared<LogicServerSession>(connectionServerConfig.id(), connectionServerConfig.logicserverloginpassword()), 10000, 32 * 1024 * 1024);
    });

    /*  同步etcd  */
    std::map<std::string, std::string> etcdKV;

    etcdKV["value"] = buffer.str();
    etcdKV["ttl"] = std::to_string(15); /*存活时间为15秒*/

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

        for (auto& etcd : connectionServerConfig.etcdservers())
        {
            if (!etcdSet(etcd.ip(), etcd.port(), std::string("ConnectionServerList/") + std::to_string(connectionServerConfig.id()), etcdKV, 5000).getBody().empty())
            {
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5000));   /*5s 重设一次*/
    }

    clientListener->closeListenThread();
    logicServerListener->closeListenThread();
    service->getService()->closeListenThread();
    service->getService()->stopWorkerThread();
    gDailyLogger->stop();

    return 0;
}