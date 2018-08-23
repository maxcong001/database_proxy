#include "tcp_session.hpp"
#include "loop_thread.hpp"
#include "logger.hpp"
#include "resp_parser.hpp"
extern thread_local std::shared_ptr<loop_thread> _loop_thread_sptr;
void TcpSession::onRead()
{
    uint32_t length = this->getInputBufferLength();
    __LOG(debug, "now there is some message in the session. len is :  " << length);
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
        __LOG(debug, "now there is a RESP message to send");
        //std::shared_ptr<TcpClient> _conn_sptr = nullptr;
        if (!_loop_thread_sptr || _loop_thread_sptr->_connection_sptr_vector.empty())
        {
            __LOG(warn, "loop thread sptr is not valid or there is not connection in this thread!");
            return;
        }

        __LOG(debug, "there is : " << _loop_thread_sptr->_connection_sptr_vector.size() << " connection in this thread");

        unsigned short retry_time = _loop_thread_sptr->_connection_sptr_vector.size();
        retry_time++;

        do
        {
            retry_time--;
            std::shared_ptr<redis_tcp_client> _conn_sptr = _loop_thread_sptr->_connection_sptr_vector[_sIdGenerater++ % _loop_thread_sptr->_connection_sptr_vector.size()];
            if (!_conn_sptr || !_conn_sptr->isConnected())
            {
                continue;
            }
            // now got a connected connection
            __LOG(debug, "now send message to redis : " << std::string(buf, buf_length) << ", length is : " << buf_length);
            if (!_conn_sptr->send(buf, buf_length))
            {
                __LOG(error, "send return fail");
            }
            else
            {
                _conn_sptr->_session_sptr_queue.push(shared_from_this());
                return;
            }

        } while (retry_time);
        __LOG(warn, "send message fail after retry");
    });

    if (std::get<0>(ret) > 0 && !this->drainInputBuffer(std::get<0>(ret)))
    {
        __LOG(error, "drain input buffer fail!");
        return;
    }
}
void TcpSession::handleEvent(short events)
{
    //bufferevent_disable(_bev_sptr.get(), EV_READ | EV_WRITE);
    //evutil_closesocket(get_fd());
    // should kill myself
    TASK_MSG msg;
    msg.type = TASK_MSG_TYPE::DEL_SESSION;
    msg.body = _fd;

    if (_loop_thread_sptr)
    {
        _loop_thread_sptr->send2loop_thread(msg);
    }
    else
    {
        __LOG(error, "loop sptr is not valid!");
    }
}
