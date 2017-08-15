#include <string>
#include "../CenterServer/CenterServerSession.h"
#include "WrapLog.h"
#include "CenterServerExtRecvOP.h"

extern WrapLog::PTR gDailyLogger;

//TODO::其他服务器的RPC也需要重新写一次,需要考虑抽象提取出来

static int add(int a, int b)
{
    //CenterServerRPCMgr::getRpcFromer()为调用者会话对象
    return a + b;
}

static void addNoneRet(int a, int b, dodo::rpc::RpcRequestInfo reqInfo)
{
    // 添加dodo::RpcRequestInfo reqInfo形参(不影响调用者调用)
    // 这里本身不返回数据(函数返回类型为void),但RPC本身是具有返回值语义的
    // 适用于需要调用其他异步操作之后(通过reqInfo)才能返回数据给调用者的情况
    // 譬如:
    /*
        auto caller = CenterServerRPCMgr::getRpcFromer();
        redis->get("k", [caller, reqInfo](const std::string& value){
            caller->reply(reqInfo, value);
        });
    */

    /*
        //客户端:
        centerServerConnectionRpc->call("test", 1, 2, [](const std::string& value){

        });
    */
}

void initCenterServerExt()
{
    CenterServerRPCMgr::def("test", [](int a, int b){
        return a + b;
    });

    CenterServerRPCMgr::def("testNoneRet", [](int a, int b){
    });

    CenterServerRPCMgr::def("add", add);

    CenterServerRPCMgr::def("addNoneRet", addNoneRet);

    /*处理来自内部服务器的rpc请求,请求者为:gCenterServerSessionRpcFromer*/
    CenterServerSessionGlobalData::getCenterServerSessionRpc()->def("testrpc", [](const std::string& a, int b, dodo::rpc::RpcRequestInfo info){
        gDailyLogger->info("rpc handle: {} : {}", a, b);
    });

    /*处理来自内部服务器发送的OP消息*/
    CenterServerSessionGlobalData::registerUserMsgHandle(static_cast<PACKET_OP_TYPE>(CENTER_SERVER_EXT_RECV_OP::CENTER_SERVER_EXT_RECV_OP_TEST),
        [](CenterServerSession::PTR&, ReadPacket& rp){
            std::string a = rp.readBinary();
            int b = rp.readINT32();
            gDailyLogger->info("test op : {} : {}", a, b);
        });
}