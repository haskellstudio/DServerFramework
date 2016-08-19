#include <string>

#include "WebSocketFormat.h"
#include "UseWebPacketSingleNetSession.h"

size_t UseWebPacketSingleNetSession::onMsg(const char* buffer, size_t len)
{
    const static char* SEC_FLAG = "Sec-WebSocket-Key: ";
    const static int SEC_LEN = sizeof("Sec-WebSocket-Key: ")-1;

    /*  先解析websocket握手    */
    if (!mConnected)
    {
        const char* f = strstr(buffer, SEC_FLAG);
        if (f != nullptr)
        {
            const char* fend = strstr(f + SEC_LEN, "\r\n");
            if (fend != nullptr)
            {
                mConnected = true;
                const char* keyStart = f + SEC_LEN;

                std::string response = WebSocketFormat::wsHandshake(std::string(keyStart, fend-keyStart));
                sendPacket(response.c_str(), response.size());
                return len;
            }
        }
        
        return 0;
    }

    const char* parse_str = buffer;
    size_t total_proc_len = 0;
    PACKET_LEN_TYPE left_len = static_cast<PACKET_LEN_TYPE>(len);

    while (true)
    {
        bool flag = false;
        if (left_len >= 10)
        {
            std::string payload;
            uint8_t opcode; /*TODO::opcode是否回传给回调函数*/
            int frameSize = 0;
            if (WebSocketFormat::wsFrameExtractBuffer(parse_str, left_len, payload, opcode, frameSize))
            {
                ReadPacket rp(parse_str, frameSize);
                rp.readINT8();
                uint32_t l = rp.readINT32();
                uint32_t cmd = rp.readINT32();
                const char* b = buffer + rp.getPos();
                rp.addPos(l - 8);
                rp.readINT8();

                procPacket(cmd, payload.c_str(), l - 8);
                total_proc_len += frameSize;
                parse_str += frameSize;
                left_len -= frameSize;
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