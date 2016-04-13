#include <string>
#include "../CenterServer/CenterServerSession.h"
#include "WrapLog.h"
#include "CenterServerExtRecvOP.h"

extern WrapLog::PTR gDailyLogger;

void initCenterServerExt()
{
    /*处理来自内部服务器的rpc请求,请求者为:gCenterServerSessionRpcFromer*/
    CenterServerSessionGlobalData::getCenterServerSessionRpc()->def("testrpc", [](const std::string& a, int b, dodo::RpcRequestInfo info){
        gDailyLogger->info("rpc handle: {} - {}", a, b);
    });

    /*处理来自内部服务器发送的OP消息*/
    CenterServerSessionGlobalData::registerUserMsgHandle(CENTER_SERVER_EXT_RECV_OP_TEST, [](CenterServerSession&, ReadPacket& rp){
        std::string a = rp.readBinary();
        int b = rp.readINT32();
    });
}