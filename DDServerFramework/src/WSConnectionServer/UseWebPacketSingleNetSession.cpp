#include <string>

#include "WebSocketFormat.h"
#include "UseWebPacketSingleNetSession.h"

UseWebPacketSingleNetSession::UseWebPacketSingleNetSession()
{
    mShakehanded = false;
}

size_t UseWebPacketSingleNetSession::onMsg(const char* buffer, size_t len)
{
    if (!mShakehanded)
    {
        const static char* SEC_FLAG = "Sec-WebSocket-Key: ";
        const static int SEC_LEN = sizeof("Sec-WebSocket-Key: ") - 1;

        /*  处理websocket握手    */
        const char* f = strstr(buffer, SEC_FLAG);
        if (f != nullptr)
        {
            const char* fend = strstr(f + SEC_LEN, "\r\n");
            if (fend != nullptr)
            {
                mShakehanded = true;
                const char* keyStart = f + SEC_LEN;

                std::string response = WebSocketFormat::wsHandshake(std::string(keyStart, fend-keyStart));
                sendPacket(response.c_str(), response.size());
                return len;
            }
        }
        
        return 0;
    }
    else
    {
        /*  处理websocket frame   */

        const char* parse_str = buffer;
        size_t total_proc_len = 0;
        PACKET_LEN_TYPE left_len = static_cast<PACKET_LEN_TYPE>(len);

        static size_t WEB_PACKET_HEAD_LEN = 14;

        while (true)
        {
            bool flag = false;
            if (left_len >= WEB_PACKET_HEAD_LEN)
            {
                std::string payload;
                auto opcode = WebSocketFormat::WebSocketFrameType::ERROR_FRAME;
                int frameSize = 0;
                if (WebSocketFormat::wsFrameExtractBuffer(parse_str, left_len, payload, opcode, frameSize))
                {
                    if (opcode == WebSocketFormat::WebSocketFrameType::TEXT_FRAME || opcode == WebSocketFormat::WebSocketFrameType::BINARY_FRAME)
                    {
                        /*  只有text和binary帧,才解析数据    */
                        BasePacketReader rp(payload.c_str(), payload.size());
                        rp.readINT8();
                        auto packetLen = rp.readUINT32();
                        if (packetLen >= WEB_PACKET_HEAD_LEN)
                        {
                            auto cmd = rp.readINT32();
                            rp.readINT32(); /*  serial id   */
                            const char* body = payload.c_str() + rp.getPos();
                            rp.addPos(packetLen - WEB_PACKET_HEAD_LEN);
                            rp.readINT8();

                            procPacket(cmd, body, packetLen - WEB_PACKET_HEAD_LEN);
                        }
                    }
                    
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

    return 0;
}