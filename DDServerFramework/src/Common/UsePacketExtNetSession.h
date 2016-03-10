#ifndef _USEPACKET_EXTNETSESSION_H
#define _USEPACKET_EXTNETSESSION_H

#include "NetThreadSession.h"
#include "UsePacketSingleNetSession.h"

class UsePacketExtNetSession : public ExtNetSession
{
public:
    UsePacketExtNetSession(BaseLogicSession::PTR logicSession) : ExtNetSession(logicSession)
    {
    }

private:
    virtual size_t     onMsg(const char* buffer, size_t len);
};

#endif