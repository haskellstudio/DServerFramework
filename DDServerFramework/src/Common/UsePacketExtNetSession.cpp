#include "packet.h"

#include "UsePacketExtNetSession.h"

size_t UsePacketExtNetSession::onMsg(const char* buffer, size_t len)
{
    const char* parse_str = buffer;
    size_t total_proc_len = 0;
    size_t left_len = len;

    while (true)
    {
        bool flag = false;
        if (left_len >= PACKET_HEAD_LEN)
        {
            PACKET_LEN_TYPE packet_len = socketendian::networkToHost32(*(PACKET_LEN_TYPE*)parse_str);
            if (left_len >= packet_len && packet_len >= PACKET_HEAD_LEN)
            {
                PACKET_OP_TYPE op = socketendian::networkToHost16(*(PACKET_OP_TYPE*)(parse_str + sizeof(PACKET_LEN_TYPE)));
                pushDataMsg2LogicMsgList(mLogicSession, parse_str, packet_len);
                total_proc_len += packet_len;
                parse_str += packet_len;
                left_len -= packet_len;
                flag = true;
            }
        }

        if (!flag)
        {
            break;
        }
    }

    return total_proc_len;
}