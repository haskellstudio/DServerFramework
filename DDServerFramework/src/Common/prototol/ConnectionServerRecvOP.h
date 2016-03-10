#ifndef _LOGICSERVER_2_CONNECTIONSERVER_H
#define _LOGICSERVER_2_CONNECTIONSERVER_H

/*  逻辑服务器发送给链接服务器的OP    */
enum CONNECTIONSERVER_RECV_OP
{
    CONNECTION_SERVER_RECV_LOGICSERVER_LOGIN,           /*  收到logic server 登陆请求   */
    CONNECTION_SERVER_RECV_PING,
    CONNECTION_SERVER_RECV_PACKET2CLIENT_BYSOCKINFO,    /*  逻辑服务器发送消息包给玩家(根据玩家的cs id和socket id)   */
    CONNECTION_SERVER_RECV_PACKET2CLIENT_BYRUNTIMEID,   /*  逻辑服务器发送消息给玩家(根据玩家的RuntimeID)    */
    CONNECTION_SERVER_RECV_IS_SETCLIENT_SLAVEID,        /*  游戏服务器请求链接服务器关联玩家的Slave ID*/
    CONNECTION_SERVER_RECV_KICKCLIENT_BYRUNTIMEID,      /*  内部服务器请求踢出某玩家    */
};

#endif