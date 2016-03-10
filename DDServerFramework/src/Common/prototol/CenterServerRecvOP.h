#ifndef _CENTERSERVER_RECV_OP_H
#define _CENTERSERVER_RECV_OP_H

enum CENTER_SERVER_RECV_OP
{
    CENTERSERVER_RECV_OP_RPC,                       /*收到内部服务器发送的RPC请求*/
    CENTERSERVER_RECV_OP_PING,
    CENTERSERVER_RECV_OP_LOGICSERVER_LOGIN,         /*logic server登陆*/
    CENTERSERVER_RECV_OP_USER,                      /*logic server发送用户自定义消息*/
};

#endif