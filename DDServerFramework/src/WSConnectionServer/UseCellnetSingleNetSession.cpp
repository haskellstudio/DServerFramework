#include "UseCellnetSingleNetSession.h"

UseCellnetPacketSingleNetSession::UseCellnetPacketSingleNetSession()
{
    mRecvSerialID = 1;
}

size_t UseCellnetPacketSingleNetSession::onMsg(const char* buffer, size_t len)
{
    const int CELLNET_PACKET_HEAD_LEN = 8;

    const char* parse_str = buffer;
    size_t total_proc_len = 0;
    PACKET_LEN_TYPE left_len = static_cast<PACKET_LEN_TYPE>(len);

    while (true)
    {
        bool flag = false;
        if (left_len >= CELLNET_PACKET_HEAD_LEN)
        {
            auto cmdID = socketendian::networkToHost32(*(uint32_t*)parse_str, false);
            auto seralID = socketendian::networkToHost16(*(uint16_t*)(parse_str + 4), false);
            
            assert(mRecvSerialID++ == seralID);

            auto packet_len = socketendian::networkToHost16(*(uint16_t*)(parse_str + 6), false);

            if (left_len >= packet_len && packet_len >= CELLNET_PACKET_HEAD_LEN)
            {
                procPacket(cmdID, parse_str + 8, packet_len - CELLNET_PACKET_HEAD_LEN);

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