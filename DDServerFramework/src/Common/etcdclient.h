#ifndef _ETCD_CLIENT_H
#define _ETCD_CLIENT_H

#include <brynet/net/WrapTCPService.h>
#include <brynet/net/http/HttpService.h>
#include <brynet/net/http/HttpParser.h>
#include <brynet/net/http/HttpFormat.h>

using namespace brynet::net;

static HTTPParser etcdHelp(const std::string& ip, int port, HttpRequest::HTTP_METHOD protocol, const std::string& url,
    const std::map<std::string, std::string>& kv, int timeout)
{
    HTTPParser result(HTTP_BOTH);

    std::mutex mtx;
    std::condition_variable cv;

    brynet::Timer::WeakPtr timer;
    auto service = std::make_shared<WrapTcpService>();
    service->startWorkThread(1);

    sock fd = ox_socket_connect(false, ip.c_str(), port);
    if (fd != SOCKET_ERROR)
    {
        service->addSession(fd, [&cv, &timer, &result, service, timeout, kv, url, protocol](const TCPSession::PTR& session) {
            HttpService::setup(session, [&cv, &timer, &result, service, timeout, session, kv, url, protocol](const HttpSession::PTR& httpSession) {
                timer = service->getService()->getRandomEventLoop()->getTimerMgr()->addTimer(std::chrono::microseconds(timeout), [session, kv, url, protocol]() {
                    session->postDisConnect();
                });

                HttpRequest request;
                request.setHost("127.0.0.1");
                request.addHeadValue("Accept", "*/*");
                request.setMethod(protocol);
                std::string keyUrl = "/v2/keys/";
                keyUrl.append(url);
                request.setUrl(keyUrl.c_str());
                if (!kv.empty())
                {
                    for (auto& v : kv)
                    {
                        request.addHeadValue(v.first.c_str(), v.second.c_str());
                    }
                    request.setContentType("application/x-www-form-urlencoded");
                }
                std::string requestStr = request.getResult();
                session->send(requestStr.c_str(), requestStr.size());

                httpSession->setCloseCallback([&cv, &timer](HttpSession::PTR session) {
                    if (timer.lock() != nullptr)
                    {
                        timer.lock()->cancel();
                    }
                    cv.notify_one();
                });
                httpSession->setHttpCallback([&cv, &result, &timer](const HTTPParser& httpParser, HttpSession::PTR session) {
                    result = httpParser;
                    session->postClose();
                    if (timer.lock() != nullptr)
                    {
                        timer.lock()->cancel();
                    }
                });
            });
        }, false,
            nullptr, 1024, false);

        std::unique_lock<std::mutex> tmp(mtx);
        cv.wait(tmp);
    }

    return result;
}

static HTTPParser etcdSet(const std::string& ip, int port, const std::string& url, const std::map<std::string, std::string>& kv, int timeout)
{
    return etcdHelp(ip, port, HttpRequest::HTTP_METHOD::HTTP_METHOD_PUT, url, kv, timeout);
}

static HTTPParser etcdGet(const std::string& ip, int port, const std::string& url, int timeout)
{
    return etcdHelp(ip, port, HttpRequest::HTTP_METHOD::HTTP_METHOD_GET, url, {}, timeout);
}

#endif