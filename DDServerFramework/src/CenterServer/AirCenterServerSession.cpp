#include "CenterServerSession.h"
#include "WrapLog.h"
#include "AirCenterServerSession.h"

extern WrapLog::PTR gDailyLogger;

enum AIR_CENTERSERVER_SESSION_SUB_RECV_OP
{
    AIR_CENTERSERVER_SESSION_SUB_RECV_TEST,
};

static void onClientRequestCreateRoom(CenterServerSession&, ReadPacket& rp)
{
    
}


void InitAirCenterServerSessionMsgHandle()
{
    CenterServerSessionGlobalData::registerUserMsgHandle(AIR_CENTERSERVER_SESSION_SUB_RECV_TEST, onClientRequestCreateRoom);
}