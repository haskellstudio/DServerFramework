#include "UsePacketSingleNetSession.h"

size_t UsePacketSingleNetSession::onMsg(const char* buffer, size_t len)
{
    const char* parse_str = buffer;
    size_t total_proc_len = 0;
    PACKET_LEN_TYPE left_len = static_cast<PACKET_LEN_TYPE>(len);

    while (true)
    {
        bool flag = false;
        if (left_len >= PACKET_HEAD_LEN)
        {
            ReadPacket rp(parse_str, left_len);

            PACKET_LEN_TYPE packet_len = rp.readPacketLen();
            if (left_len >= packet_len && packet_len >= PACKET_HEAD_LEN)
            {
                PACKET_OP_TYPE op = rp.readOP();
                procPacket(op, parse_str + PACKET_HEAD_LEN, packet_len - PACKET_HEAD_LEN);
                total_proc_len += packet_len;
                parse_str += packet_len;
                left_len -= packet_len;
                flag = true;
            }
            rp.skipAll();
        }

        if (!flag)
        {
            break;
        }
    }

    return total_proc_len;
}