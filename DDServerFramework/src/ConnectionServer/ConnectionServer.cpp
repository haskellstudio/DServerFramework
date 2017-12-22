#include <set>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

#include <brynet/net/WrapTCPService.h>
#include <brynet/timer/Timer.h>
#include <brynet/utils/app_status.h>
#include <brynet/utils/ox_file.h>

#include "../Common/etcdclient.h"
#include "../Common/SocketSession.h"

#include <brynet/utils/packet.h>
#include "WrapLog.h"
#include "AutoConnectionServer.h"
#include "WrapJsonValue.h"
#include "ClientSession.h"
#include "LogicServerSession.h"
#include "../../ServerConfig/ServerConfig.pb.h"
#include "google/protobuf/util/json_util.h"

WrapLog::PTR                            gDailyLogger;

/*
    ���ӷ��������ڷ������ܹ��д���N������
    1�������ַ����etcd��Ⱥ. (���ڵľ�����������Է���etcd����ȡ���е����ӷ�������ַ��������һ�����Ʒ�����ͻ���)��
    2��������ͻ��˺��ڲ��߼������������ӣ�ת���ͻ��˺��ڲ�������֮���ͨ��
*/

static void loop(const std::stringstream& buffer, const ServerConfig::ConnectionServerConfig& connectionServerConfig)
{
    /*  ͬ��etcd  */
    std::map<std::string, std::string> etcdKV;

    etcdKV["value"] = buffer.str();
    etcdKV["ttl"] = std::to_string(15); /*���ʱ��Ϊ15��*/

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

        std::this_thread::sleep_for(std::chrono::milliseconds(5000));   /*5s ����һ��*/
    }
}

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
    
    service->startWorkThread(std::thread::hardware_concurrency(), [](EventLoop::PTR) {
    });

    /*��������ͻ��˶˿�*/
    auto clientListener = ListenThread::Create();
    clientListener->startListen(connectionServerConfig.enableipv6(), connectionServerConfig.bindip(), connectionServerConfig.portforclient(), [=](sock fd) {
        service->addSession(fd, [service](const TCPSession::PTR& session) {
            //connection->setSession(service, session, session->getIP());
            //connection->onEnter();

            // �����߼�����
            auto ss = std::make_shared<SocketSession>();
            auto logicSession = std::shared_ptr<ConnectionClientSession>();

            logicSession->onEnter();

            session->setHeartBeat(std::chrono::nanoseconds::zero());

            session->setDisConnectCallback([logicSession](const TCPSession::PTR& session) {
                logicSession->onClose();
            });

            // TODO::�����Ҳ��Ϊhandle��һ��������,������Ҫ�޸���������Ϣ���ջص�(��ΪĿǰ�Ļص���Ҫ����ֵ)
            session->setDataCallback([ss](const TCPSession::PTR& session, const char* buffer, size_t len) {
                const char* parse_str = buffer;
                size_t total_proc_len = 0;
                PACKET_LEN_TYPE left_len = static_cast<PACKET_LEN_TYPE>(len);

                while (true)
                {
                    bool flag = false;
                    if (left_len >= PACKET_HEAD_LEN)
                    {
                        ReadPacket rp(parse_str, left_len);

                        PACKET_LEN_TYPE packet_len = rp.readPacketLen();
                        if (left_len >= packet_len && packet_len >= PACKET_HEAD_LEN)
                        {
                            PACKET_OP_TYPE op = rp.readOP();

                            ss->handle(std::string(parse_str, packet_len));
                            total_proc_len += packet_len;
                            parse_str += packet_len;
                            left_len -= packet_len;
                            flag = true;
                        }
                        rp.skipAll();
                    }

                    if (!flag)
                    {
                        break;
                    }
                }

                return total_proc_len;
            });

            ss->childHandler([logicSession](Context& context, const std::string& buffer, const SocketSession::INTERCEPTOR& next) {
                ReadPacket rp(buffer.c_str(), buffer.size());
                logicSession->procPacket(rp.readOP(), rp.getBuffer(), rp.getPos());
                next(context, buffer);
            });
        }, false, nullptr, 32 * 1024);
    });

    /*���������߼��������˿�*/
    auto logicServerListener = ListenThread::Create();
    logicServerListener->startListen(connectionServerConfig.enableipv6(), connectionServerConfig.bindip(), connectionServerConfig.portforlogicserver(), [=](sock fd) {
        service->addSession(fd, [service](const TCPSession::PTR& session) {

            // �����߼�����
            auto ss = std::make_shared<SocketSession>();
            auto logicSession = std::shared_ptr<LogicServerSession>();

            logicSession->onEnter();

            session->setHeartBeat(std::chrono::nanoseconds::zero());

            session->setDisConnectCallback([logicSession](const TCPSession::PTR& session) {
                logicSession->onClose();
            });

            // TODO::�����Ҳ��Ϊhandle��һ��������,������Ҫ�޸���������Ϣ���ջص�(��ΪĿǰ�Ļص���Ҫ����ֵ)
            session->setDataCallback([ss](const TCPSession::PTR& session, const char* buffer, size_t len) {
                const char* parse_str = buffer;
                size_t total_proc_len = 0;
                PACKET_LEN_TYPE left_len = static_cast<PACKET_LEN_TYPE>(len);

                while (true)
                {
                    bool flag = false;
                    if (left_len >= PACKET_HEAD_LEN)
                    {
                        ReadPacket rp(parse_str, left_len);

                        PACKET_LEN_TYPE packet_len = rp.readPacketLen();
                        if (left_len >= packet_len && packet_len >= PACKET_HEAD_LEN)
                        {
                            PACKET_OP_TYPE op = rp.readOP();

                            ss->handle(std::string(parse_str, packet_len));
                            total_proc_len += packet_len;
                            parse_str += packet_len;
                            left_len -= packet_len;
                            flag = true;
                        }
                        rp.skipAll();
                    }

                    if (!flag)
                    {
                        break;
                    }
                }

                return total_proc_len;
            });

            ss->childHandler([logicSession](Context& context, const std::string& buffer, const SocketSession::INTERCEPTOR& next) {
                ReadPacket rp(buffer.c_str(), buffer.size());
                logicSession->procPacket(rp.readOP(), rp.getBuffer(), rp.getPos());
                next(context, buffer);
            });
        }, false, nullptr, 32 * 1024);
    });

    loop(buffer, connectionServerConfig);

    clientListener->stopListen();
    logicServerListener->stopListen();
    service->getService()->stopWorkerThread();
    service->getService()->stopWorkerThread();
    gDailyLogger->stop();

    return 0;
}