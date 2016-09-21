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
    auto left_len = len;

    while (true)
    {
        bool flag = false;
        if (left_len >= CELLNET_PACKET_HEAD_LEN)
        {
            BasePacketReader bp(parse_str, left_len, false);

            auto cmdID = bp.readUINT32();
            auto seralID = bp.readUINT16();
            
            assert(mRecvSerialID++ == seralID);

            auto packet_len = bp.readUINT16();
            
            assert(bp.getPos() == CELLNET_PACKET_HEAD_LEN);

            if (left_len >= packet_len && packet_len >= CELLNET_PACKET_HEAD_LEN)
            {
                procPacket(cmdID, bp.getBuffer() + bp.getPos(), packet_len - bp.getPos());

                total_proc_len += packet_len;
                parse_str += packet_len;
                left_len -= packet_len;
                flag = true;
            }

            bp.skipAll();
        }

        if (!flag)
        {
            break;
        }
    }

    return total_proc_len;
}