#ifndef _CONNECTIONSERVER_2_LOGICSERVER_H
#define _CONNECTIONSERVER_2_LOGICSERVER_H

/*  链接服务器发送给逻辑服务器的消息包   */
enum CONNECTION_SERVER_SEND_OP
{
    CONNECTION_SERVER_SEND_PONG,
    CONNECTION_SERVER_SEND_LOGICSERVER_CLIENT_DISCONNECT,   /*  链接服务器通知逻辑服务器某玩家网络离线(需要等待断线重连) */
    //放入user msg 中处理 (包括ping）CONNECTION_SERVER_SEND_LOGICSERVER_CLIENT_RECONNECT,    /*  链接服务器通知逻辑服务器某玩家断线重连进来   */
    CONNECTION_SERVER_SEND_LOGICSERVER_RECVCSID,            /*  链接服务器将自身id发送给solo服务器    */
    CONNECTION_SERVER_SEND_LOGICSERVER_INIT_CLIENTMIRROR,   /*  通知solo服务器初始化某玩家session  */
    CONNECTION_SERVER_SEND_LOGICSERVER_DESTROY_CLIENT,      /*  通知逻辑服务器强制删除玩家   */
    CONNECTION_SERVER_SEND_LOGICSERVER_FROMCLIENT,          /*  链接服务器转发玩家的消息包给逻辑服务器 */
};

#endif