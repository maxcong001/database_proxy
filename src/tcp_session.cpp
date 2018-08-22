#include "tcp_session.hpp"
#include "loop_thread.hpp"

#include "resp_parser.hpp"
void TcpSession::onRead()
{
    __LOG(debug, "now there is some message in the session");
    uint32_t length = this->getInputBufferLength();
    const uint8_t *buf = this->viewInputBuffer(length);
    auto ret = rasp_parser::process_resp((char *)(const_cast<uint8_t *>(buf)), length, [this](char *buf, size_t buf_length) {
        __LOG(debug, "now there is a RESP message to send");
        std::shared_ptr<TcpClient> _conn_sptr = nullptr;
        unsigned short retry_time = loop_thread::_connection_sptr_vector.size();
        retry_time++;
        do
        {
            retry_time--;
            _conn_sptr = loop_thread::_connection_sptr_vector[_sIdGenerater++ / loop_thread::_connection_sptr_vector.size()];
            if (!_conn_sptr || !_conn_sptr->isConnected())
            {
                continue;
            }
            // now got a connected connection
            if (!_conn_sptr->send(buf, buf_length))
            {
                __LOG(error, "send return fail");
            }
        } while (retry_time);
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

    if (loop_thread::_loop_thread_sptr)
    {
        loop_thread::_loop_thread_sptr->send2loop_thread(msg);
    }
    else
    {
        __LOG(error, "loop sptr is not valid!");
    }
}
