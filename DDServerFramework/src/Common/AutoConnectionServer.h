#ifndef _AUTO_CONNECTION_SERVER_H
#define _AUTO_CONNECTION_SERVER_H

#include <string>
#include <thread>

#include "WrapTCPService.h"
#include "WrapLog.h"
#include "UsePacketExtNetSession.h"

const static int AUTO_CONNECT_DELAY = 5000;

template<typename T, typename TT>
struct MyFuck
{
    static void foo(WrapServer::PTR server, sock fd)
    {
        WrapAddNetSession(server, fd, make_shared<T>(std::make_shared<TT>()), 10000, 32 * 1024 * 1024);
    }
};

template<typename T>
struct MyFuck<T, void>
{
    static void foo(WrapServer::PTR server, sock fd)
    {
        WrapAddNetSession(server, fd, make_shared<T>(), 10000, 32 * 1024 * 1024);
    }
};

template<typename T, typename TT>
void    autoConnectServer(WrapLog::PTR log, WrapServer::PTR server, std::string ip, int port)
{
    log->warn("start connect {}-{} : {} : {}", typeid(T).name(), typeid(TT).name(), ip, port);
    sock fd = ox_socket_connect(false, ip.c_str(), port);
    if (fd != SOCKET_ERROR)
    {
        log->warn("connect success");
        MyFuck<T, TT>::foo(server, fd);
    }
    else
    {
        log->warn("connect failed, sleep {} s, will reconnect", AUTO_CONNECT_DELAY/1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(AUTO_CONNECT_DELAY));
        startConnectThread<T, TT>(log, server, ip, port);
    }
}

template<typename T, typename TT>
void    startConnectThread(WrapLog::PTR log, WrapServer::PTR server, std::string ip, int port)
{
    std::thread(autoConnectServer<T, TT>, log, server, ip, port).detach();
}

#endif