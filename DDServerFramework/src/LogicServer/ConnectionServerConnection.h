#ifndef _CONNECTIONSERVER_CONNECTION_H
#define _CONNECTIONSERVER_CONNECTION_H

#include <memory>
#include <unordered_map>
#include <stdint.h>


class Packet;

/*链接到链接服务器*/
class ConnectionServerConnection : public std::enable_shared_from_this<ConnectionServerConnection>
{
public:
    typedef std::shared_ptr<ConnectionServerConnection> PTR;
    typedef std::weak_ptr<ConnectionServerConnection> WEAK_PTR;

    ConnectionServerConnection(int idInEtcd, int port, std::string password);
    ~ConnectionServerConnection();

    void            sendPacket(Packet&);
    
    static ConnectionServerConnection::PTR FindConnectionServerByID(int32_t id);

     void    onEnter() ;
     void    onClose() ;
     void    onMsg(const char* data, size_t len) ;

    void            ping();
    void            initClient(int64_t runtimeID, int64_t socketID);

private:
    bool            mIsSuccess;
    int             mPort;
    int             mIDInEtcd;
    int32_t         mConnectionServerID;
    std::string     mPassword;
    brynet::Timer::WeakPtr  mPingTimer;
};

void    tryCompareConnect(std::unordered_map<int32_t, std::tuple<std::string, int, std::string>>& servers);

#endif