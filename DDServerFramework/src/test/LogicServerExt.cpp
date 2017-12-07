#include <string>

#include <brynet/utils/packet.h>

#include "../LogicServer/ClientMirror.h"
#include "../LogicServer/CenterServerConnection.h"
#include "../LogicServer/GlobalValue.h"

#include "ClientExtOP.h"
#include "CenterServerExtRecvOP.h"

void initLogicServerExt()
{
    /*处理客户端发送的消息*/
    gClientMirrorMgr->registerUserMsgHandle(static_cast<PACKET_OP_TYPE>(CLIENT_OP::CLIENT_OP_TEST), [](ClientMirror::PTR& client, ReadPacket& rp){
        client->sendv(static_cast<PACKET_OP_TYPE>(CLIENT_OP::CLIENT_OP_TEST), 1);
        if (gLogicCenterServerClient != nullptr)
        {
            std::string a = rp.readBinary();
            std::cout << " test op, a  is " << a << std::endl;
            gLogicCenterServerClient->sendUserV(static_cast<PACKET_OP_TYPE>(CENTER_SERVER_EXT_RECV_OP::CENTER_SERVER_EXT_RECV_OP_TEST), "haha", 2);
            gLogicCenterServerClient->call("testrpc", "heihei", 3);
        }
    });

}