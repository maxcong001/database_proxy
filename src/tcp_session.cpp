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
long int TcpSession::bufToLong(const char *str, size_t size)
{
    long int value = 0;
    bool sign = false;

    if (str == nullptr || size == 0)
    {
        return 0;
    }

    if (*str == '-')
    {
        sign = true;
        ++str;
        --size;

        if (size == 0)
        {
            return 0;
        }
    }

    for (const char *end = str + size; str != end; ++str)
    {
        char c = *str;

        // char must be valid, already checked in the parser
        assert(c >= '0' && c <= '9');

        value = value * 10;
        value += c - '0';
    }

    return sign ? -value : value;
}

void TcpSession::onRead()
{

    uint32_t length = this->getInputBufferLength();
    const uint8_t *buf = this->viewInputBuffer(length);

    auto ret = rasp_parser::process_resp(buf, length, [this](char *buf, size_t buf_length) {
        auto context_sptr = loop_thread::_context_sptr_vector[_sIdGenerater++];
        redisAsyncFormattedCommand(context_sptr.get(), hiredisCallback, (void *)(unsigned long(_fd)), buf, buf_length);
    });

    if (std::get<0>(ret) > 0 && !this->drainInputBuffer(std::get<0>(ret)))
    {
        __LOG(error, "drain input buffer fail!");
        return;
    }
}
