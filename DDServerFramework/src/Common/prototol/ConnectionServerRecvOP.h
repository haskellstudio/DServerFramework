#ifndef _LOGICSERVER_2_CONNECTIONSERVER_H
#define _LOGICSERVER_2_CONNECTIONSERVER_H

/*  �߼����������͸����ӷ�������OP    */
enum CONNECTIONSERVER_RECV_OP
{
    CONNECTION_SERVER_RECV_LOGICSERVER_LOGIN,           /*  �յ�logic server ��½����   */
    CONNECTION_SERVER_RECV_PING,
    CONNECTION_SERVER_RECV_PACKET2CLIENT_BYSOCKINFO,    /*  �߼�������������Ϣ�����ͻ���(���ݿͻ��˵�cs id��socket id)   */
    CONNECTION_SERVER_RECV_PACKET2CLIENT_BYRUNTIMEID,   /*  �߼�������������Ϣ���ͻ���(���ݿͻ��˵�RuntimeID)    */
    CONNECTION_SERVER_RECV_IS_SETCLIENT_SLAVEID,        /*  ��Ϸ�������������ӷ����������ͻ��˵�Slave ID  */
    CONNECTION_SERVER_RECV_IS_SETCLIENT_PRIMARYID,      /*  ��Ϸ�������������ӷ��������ÿͻ��˵�Primary ID,ͨ�����ڶ�������ʱ  */
    CONNECTION_SERVER_RECV_KICKCLIENT_BYRUNTIMEID,      /*  �ڲ������������߳�ĳ�ͻ���    */
};

#endif