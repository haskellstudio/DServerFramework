#ifndef _CENTERSERVER_PASSWORD_H
#define _CENTERSERVER_PASSWORD_H

#include <string>
#include <exception>

#include "lua_tinker.h"

class CenterServerPassword
{
public:
    static  CenterServerPassword&   getInstance()
    {
        static CenterServerPassword sThis;
        return sThis;
    }

    void            load(lua_State* lua)
    {
        if (!lua_tinker::dofile(lua, "ServerConfig//CenterServerPassword.lua"))
        {
            throw std::runtime_error(string("ServerConfig//CenterServerPassword.lua") + "not found");
        }

        mPassword = lua_tinker::get<string>(lua, "gLoginCenterServerPassword");
    }

    const string&   getPassword() const
    {
        return mPassword;
    }
private:
    std::string     mPassword;
};


#endif