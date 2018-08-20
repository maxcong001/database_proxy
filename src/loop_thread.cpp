std::thread_local std::shared_ptr<loop_thread> loop_thread::_loop_thread_sptr = nullptr;
std::thread_local std::map<evutil_socket_t, std::shared_ptr<TcpSession>> loop_thread::_fd_to_session_map;
std::thread_local std::vector<std::shared_ptr<TcpClient>> loop_thread::_connection_sptr_vector;

void connectCallback(const redisAsyncContext *c, int status)
{
    if (status != REDIS_OK)
    {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext *c, int status)
{
    if (status != REDIS_OK)
    {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Disconnected...\n");
}

void loop_thread::process_msg(uint64_t num)
{
    TASK_QUEUE _tmp_task_queue;
    //__LOG(debug, "task with id : " << _evfd << " receive message");
    {
        std::lock_guard<std::mutex> lck(mtx);
        // actually process all the messages
        swap(_task_queue, _tmp_task_queue);
        if (_tmp_task_queue.empty())
        {
            __LOG(warn, "_tmp_task_queue queue is empty");
            return;
        }
    }
    while (_tmp_task_queue.size() != 0)
    {
        auto tmp = _tmp_task_queue.front();

        switch (tmp.type)
        {
        case NEW_SESSION:
        {
            evutil_socket_t socket_fd;
            try
            {
                socket_fd = TASK_ANY_CAST<evutil_socket_t>(tmp.body);
            }
            catch (std::exception &e)
            {
                _tmp_task_queue.pop();
                __LOG(error, "!!!!!!!!!!!!exception happend when trying to cast message, info :" << e.what());
                continue;
            }

            std::shared_ptr<TcpSession> _session_sptr(socket_fd, _loop_sptr->get_base_sptr());
            loop_thread::_fd_to_session_map[socket_fd] = _session_sptr;
        }
        break;
        case DEL_SESSION:
        {
            evutil_socket_t socket_fd;
            try
            {
                socket_fd = TASK_ANY_CAST<evutil_socket_t>(tmp.body);
            }
            catch (std::exception &e)
            {
                _tmp_task_queue.pop();
                __LOG(error, "!!!!!!!!!!!!exception happend when trying to cast message, info :" << e.what());
                continue;
            }
            loop_thread::_fd_to_session_map.erase(socket_fd);
        }
        break;
        case EXIT_LOOP:
        {
            stop();
        }
        break;
        case ADD_CONN:
        {
            CONN_INFO _conn_info;
            try
            {
                _conn_info = TASK_ANY_CAST<CONN_INFO>(tmp.body);
            }
            catch (std::exception &e)
            {
                _tmp_task_queue.pop();
                __LOG(error, "!!!!!!!!!!!!exception happend when trying to cast message, info :" << e.what());
                continue;
            }
            if (_conn_info.type == CONN_TYPE::IP_V4 || _conn_info.type == CONN_TYPE::IP_V6)
            {
                std::shared_ptr<TcpClient> _conn_sptr(get_loop());
                _conn_sptr->connect(_conn_info);
                loop_thread::_connection_sptr_vector.push_back(_conn_sptr);
            }
            else if (_conn_info.type == CONN_TYPE::UNIX_SOCKET)
            {
                std::shared_ptr<TcpClient> _conn_sptr(get_loop());
                _conn_sptr->connect(_conn_info);
                loop_thread::_connection_sptr_vector.push_back(_conn_sptr);
            }
            else
            {
                __LOG(error, "unsupported conn type!");
                _tmp_task_queue.pop();
                continue;
            }
        }
        break;
        case DEL_CONN:
        {
            // del one connection
        }
        break;
        default:
            __LOG(error, "unsupport message type");
            break;
        }
        _tmp_task_queue.pop();
    }
}
void hiredisCallback(redisAsyncContext *c, void *r, void *privdata)
{
    redisReply *reply = r;
    if (reply == NULL)
        return;
    printf("argv[%s]: %s\n", (char *)privdata, reply->str);

    /* Disconnect after receiving the reply to GET */
    redisAsyncDisconnect(c);
}