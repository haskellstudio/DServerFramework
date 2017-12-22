#ifndef _SOCKET_SESSION_H
#define _SOCKET_SESSION_H

#include <memory>
#include <functional>
#include <string>

#include "context.h"

class SocketSession
{
public:
    typedef std::function<void(Context&, const std::string&)> INTERCEPTOR;

    // ´¦ÀíÆ÷
    typedef std::function<void(Context&, const std::string&, const INTERCEPTOR&)> HANDLER;

    SocketSession& childHandler(HANDLER handler)
    {
        auto newInterceptor = std::make_shared<INTERCEPTOR>([](Context&, const std::string& buffer) {
        });

        auto wrapper = [=](Context& context, const std::string& buffer) {
            handler(context, buffer, *newInterceptor);
        };

        if (mLastInterceptor == nullptr)
        {
            mLastInterceptor = std::make_shared<INTERCEPTOR>();
        }
        else
        {
            *mLastInterceptor = wrapper;
        }
        mLastInterceptor = newInterceptor;

        if (mReceiveChain == nullptr)
        {
            mReceiveChain = wrapper;
        }

        return *this;
    }

    void handle(const std::string& buffer)
    {
        if (mReceiveChain != nullptr)
        {
            mReceiveChain(context, buffer);
            context.clear();
        }
    }

private:
    Context context;
    INTERCEPTOR mReceiveChain;
    std::shared_ptr<INTERCEPTOR> mLastInterceptor;
};


#endif