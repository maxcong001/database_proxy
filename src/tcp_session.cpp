void hiredisCallback(redisAsyncContext *c, void *r, void *privdata)
{
    redisReply *reply = r;
    if (reply == NULL)
    {
        return;
    }
    uint32_t len;
    char *buffer = hiredis_reply_to_buffer(len);
    auto session = loop_thread::_fd_to_session_map[(evutil_socket_t)((unsigned long)privdata)];
    session->send(buffer, len);
}


void TcpSession::onRead()
{
    uint32_t length = this->getInputBufferLength();
    const uint8_t *buf = this->viewInputBuffer(length);
    auto ret = rasp_parser::process_resp(buf, length, [this](char *buf, size_t buf_length) {
        std::shared_ptr<TcpClient> _conn_sptr = nullptr;
        unsigned short retry_time = loop_thread::_connection_sptr_vector.size();
        while(!_conn_sptr = loop_thread::_connection_sptr_vector[_sIdGenerater++ / loop_thread::_connection_sptr_vector.size()] || !_conn_sptr->isConnected())
        {
            retry_time--;
            if(!retry_time)
            {
                __LOG(warn, "there is no connection!");
                return ;
            }
        }
        // now got a connected connection
        if(!_conn_sptr->send(buf, buf_length))
        {
            __LOG(error, "send return fail");
        }
    });

    if (std::get<0>(ret) > 0 && !this->drainInputBuffer(std::get<0>(ret)))
    {
        __LOG(error, "drain input buffer fail!");
        return;
    }
}
