#ifndef _CONNECTIONSERVER_PASSWORD_H
#define _CONNECTIONSERVER_PASSWORD_H

#include <string>
#include <exception>

#include "lua_tinker.h"

class ConnectionServerPassword
{
public:
    static  ConnectionServerPassword&   getInstance()
    {
        static ConnectionServerPassword sThis;
        return sThis;
    }

    void            load(lua_State* lua)
    {
        if (!lua_tinker::dofile(lua, "ServerConfig//ConnectionServerPassword.lua"))
        {
            throw std::runtime_error(string("ServerConfig//ConnectionServerPassword.lua") + "not found");
        }

        mPassword = lua_tinker::get<string>(lua, "gLoginConnectionServerPassword");
    }

    const string&   getPassword() const
    {
        return mPassword;
    }
private:
    std::string     mPassword;
};


#endif