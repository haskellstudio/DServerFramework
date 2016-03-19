#include <string>

#include "../LogicServer/ClientMirror.h"
#include "../LogicServer/CenterServerConnection.h"

#include "packet.h"

#include "ClientExtOP.h"
#include "CenterServerExtRecvOP.h"

void initLogicServerExt()
{
    /*处理客户端发送的消息*/
    ClientMirror::registerUserMsgHandle(CLIENT_OP_TEST, [](ClientMirror::PTR&, ReadPacket& rp){
        std::string a = rp.readBinary();
        if (gLogicCenterServerClient != nullptr)
        {
            gLogicCenterServerClient->sendUserV(CENTER_SERVER_EXT_RECV_OP_TEST, "haha", 2);
            gLogicCenterServerClient->call("testrpc", "heihei", 3);
        }
    });

}