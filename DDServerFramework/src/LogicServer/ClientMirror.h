#ifndef _LOGIC_CLIENT_MIRROR_H
#define _LOGIC_CLIENT_MIRROR_H

#include <stdint.h>
#include <unordered_map>
#include <memory>
#include <functional>

#include <brynet/utils/packet.h>
#include "ConnectionServerConnection.h"

class ReadPacket;
class Packet;

/*  TODO::��ʱ�Ƴ�    */
/*  logic server�ϵĿͻ������羵�� */
class ClientMirror : public std::enable_shared_from_this<ClientMirror>
{
public:
    typedef std::shared_ptr<ClientMirror>   PTR;

    ClientMirror(int64_t runtimeID, int csID, int64_t socketID);
    ~ClientMirror();


    int32_t         getConnectionServerID() const;
    int64_t         getRuntimeID() const;

    void            sendPacket(Packet& realPacket) const;
    void            sendPacket(const std::string& realPacketBinary) const;
    void            sendPacket(const char* buffer, size_t len) const;

    template<typename... Args>
    void            sendv(PACKET_OP_TYPE op, const Args&... args) const
    {
        BigPacket packet(op);
        packet.writev(args...);
        sendPacket(packet);
    }

    /*  ����ͻ����������ӷ���������(��ȡ������)��ǰ�ͻ��˵�slaveΪ��ǰlogic server*/
    void            requestConnectionServerSlave(bool isSet) const;

private:
    void            procData(const char* buffer, size_t len);
    void            sendToConnectionServer(Packet& packet) const;

private:
    const int64_t   mRuntimeID;                         /*  �˿ͻ�������Ϸ����ʱ��ID    */
    const int32_t   mConnectionServerID;                /*  �˿ͻ����������ӷ�������ID   */
    const int64_t   mSocketIDOnConnectionServer;        /*  �˿ͻ������������ӷ������ϵ�socketid   */

    ConnectionServerConnection::WEAK_PTR mConnectionServer;

private:

    friend class ConnectionServerConnection;
};

class ClientMirrorMgr
{
public:
    typedef std::shared_ptr<ClientMirrorMgr> PTR;
    /*key Ϊ�ͻ��˵�RuntimeID*/
    typedef std::unordered_map<int64_t, ClientMirror::PTR> CLIENT_MIRROR_MAP;

    typedef std::function<void(ClientMirror::PTR)> ENTER_HANDLE;
    typedef std::function<void(ClientMirror::PTR)> DISCONNECT_HANDLE;
    typedef std::function<void(ClientMirror::PTR&, ReadPacket&)>   USER_MSG_HANDLE;

public:
    ClientMirror::PTR                           FindClientByRuntimeID(int64_t id);
    void                                        AddClientOnRuntimeID(ClientMirror::PTR p, int64_t id);
    void                                        DelClientByRuntimeID(int64_t id);
    CLIENT_MIRROR_MAP&                          getAllClientMirror();

    void                                        setClientEnterCallback(ENTER_HANDLE);
    void                                        setClientDisConnectCallback(DISCONNECT_HANDLE);

    ENTER_HANDLE                                getClientEnterCallback();
    DISCONNECT_HANDLE                           getClientDisConnectCallback();
    void                                        registerUserMsgHandle(PACKET_OP_TYPE, USER_MSG_HANDLE);
    USER_MSG_HANDLE                             findUserMsgHandle(PACKET_OP_TYPE);

private:
    CLIENT_MIRROR_MAP                                   mAllClientMirrorOnRuntimeID;
    std::unordered_map<PACKET_OP_TYPE, USER_MSG_HANDLE> mUserMsgHandlers;       /*  �û�ҵ���߼���Ϣ��������   */
    ENTER_HANDLE                                        mEnterHandle;           /*  �ͻ��˵Ľ���ص�    */
    DISCONNECT_HANDLE                                   mDisConnectHandle;      /*  �ͻ��˵ĶϿ��ص�*/
};

#endif