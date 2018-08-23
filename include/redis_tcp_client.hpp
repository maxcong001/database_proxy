#pragma once
#include "tcpClient.h"
#include "tcp_session.hpp"
class redis_tcp_client : public TcpClient
{
  public:
    redis_tcp_client() = delete;
    redis_tcp_client(std::shared_ptr<Loop> loop) : TcpClient(loop)
    {
    }
    void onRead() override
    {
        uint32_t length = this->getInputBufferLength();

        if (length <= 0)
        {
            __LOG(debug, "read : " << length << " byte");
            return;
        }

        const uint8_t *buf = this->viewInputBuffer(length);
        if (!buf)
        {
            __LOG(debug, "buf is not valid, ptr is : " << (void *)buf);
            return;
        }
        __LOG(debug, "data in the buf is : " << std::string((char *)(const_cast<uint8_t *>(buf)), length));

        auto ret = rasp_parser::process_resp((char *)(const_cast<uint8_t *>(buf)), length, [this](char *buf, size_t buf_length) {
            __LOG(debug, "now there is a RESP message for redis TCP client");
            std::shared_ptr<TcpSession> session = _session_sptr_queue.front();
            __LOG(debug, "now send message to tcp client, message is : " << std::string(buf, buf_length));
            session->send(buf, buf_length);
            _session_sptr_queue.pop();
        });

        if (std::get<0>(ret) > 0 && !this->drainInputBuffer(std::get<0>(ret)))
        {
            __LOG(error, "drain input buffer fail!");
            return;
        }
    };

    std::queue<std::shared_ptr<TcpSession>> _session_sptr_queue;
};