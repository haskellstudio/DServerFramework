#include <string>

#include "../LogicServer/ClientMirror.h"
#include "../LogicServer/CenterServerConnection.h"

#include "packet.h"

#include "ClientExtOP.h"
#include "CenterServerExtRecvOP.h"

void initLogicServerExt()
{
    /*  对于重连,使用client的runtimeID查找逻辑层的User对象,然后设置User对象中的ClientMirror智能指针即可,并重新映射User逻辑对象和RuntimeID的关系    */
    /*  不要将ClientMirror作为逻辑对象,TODO::为它添加ud参数,避免二次查找(逻辑对象)   */
    /*  对于slave服务器，需要手动（由用户编写代码）由primary服务器通过中心服务器告诉它，进行创建ClientMirror对象。建立映射，然后使用此对象的requestConnectionServerSlave接口通过cs设置slave    */
    /*处理客户端发送的消息*/
    ClientMirror::registerUserMsgHandle(CLIENT_OP_TEST, [](ClientMirror::PTR& client, ReadPacket& rp){
        client->sendv(CLIENT_OP_TEST, 1);
        if (false && gLogicCenterServerClient != nullptr)
        {
            std::string a = rp.readBinary();
            gLogicCenterServerClient->sendUserV(CENTER_SERVER_EXT_RECV_OP_TEST, "haha", 2);
            gLogicCenterServerClient->call("testrpc", "heihei", 3);
        }
    });

}