#include "loop_thread.hpp"
#include "tcp_listener.hpp"
thread_local std::shared_ptr<loop_thread> _loop_thread_sptr;

void loop_thread::process_msg(uint64_t num)
{
    __LOG(debug, "receive [" << num << "] message from eventfd")
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
        case TASK_MSG_TYPE::NEW_SESSION:
        {
            __LOG(debug, "now try to add a new session");
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
            __LOG(debug, "now try to add a new session, socket fd is : " << socket_fd);
            //evutil_make_socket_nonblocking(socket_fd);
            std::shared_ptr<TcpSession> _session_sptr(new TcpSession(socket_fd, _loop_sptr->get_base_sptr()));
            _session_sptr->init();
            _fd_to_session_map[socket_fd] = _session_sptr;
        }
        break;
        case TASK_MSG_TYPE::NEW_LISTENER:
        {
            __LOG(debug, "now try to start a new listener");
            CONN_INFO conn;
            try
            {
                conn = TASK_ANY_CAST<CONN_INFO>(tmp.body);
            }
            catch (std::exception &e)
            {
                _tmp_task_queue.pop();
                __LOG(error, "!!!!!!!!!!!!exception happend when trying to cast message, info :" << e.what());
                continue;
            }
            // just add it, will add some more code to del function
            TCPListener *listener_ptr = new TCPListener(_loop_sptr->get_base_sptr());
            listener_ptr->listen(conn.IP, conn.port);
        }
        break;

        case TASK_MSG_TYPE::DEL_SESSION:
        {
            __LOG(debug, "receive a delete session message from eventfd");
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
        case TASK_MSG_TYPE::EXIT_LOOP:
        {
            stop();
        }
        break;
        case TASK_MSG_TYPE::ADD_CONN:
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
            __LOG(debug, "now add a new connection, IP : " << _conn_info.IP << ", port is :" << _conn_info.port);
            if (_conn_info.type == CONN_TYPE::IP_V4 || _conn_info.type == CONN_TYPE::IP_V6)
            {
                std::shared_ptr<redis_tcp_client> _conn_sptr(new redis_tcp_client(get_loop()));
                _conn_sptr->init();
                _conn_sptr->connect(_conn_info);
                _connection_sptr_vector.push_back(_conn_sptr);
            }
            else if (_conn_info.type == CONN_TYPE::UNIX_SOCKET)
            {
                std::shared_ptr<redis_tcp_client> _conn_sptr(new redis_tcp_client(get_loop()));
                _conn_sptr->connect(_conn_info);
                _conn_sptr->init();
                _connection_sptr_vector.push_back(_conn_sptr);
            }
            else
            {
                __LOG(error, "unsupported conn type!");
                _tmp_task_queue.pop();
                continue;
            }
        }
        break;
        case TASK_MSG_TYPE::DEL_CONN:
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

bool loop_thread::init(bool new_thread)
{
    _loop_sptr = std::make_shared<Loop>();
    if (!_loop_sptr)
    {
        __LOG(error, "create loop return error!");
        return false;
    }
    if (!_loop_sptr->init())
    {
        __LOG(error, "init loop return fail!");
        return false;
    }

    _evfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

    if (_evfd <= 0)
    {
        __LOG(error, "!!!!!!!!create event fd fail!");
        return false;
    }
    __LOG(debug, "init task with eventfd ID :" << _evfd);
    // start eventfd server
    event_base *_event_base = NULL;
    _event_base = _loop_sptr->ev();
    // check event base is ready, if not ready sleep 10ms
    // at some rainy-day case, event base maybe NULL
    if (!_event_base)
    {
        __LOG(warn, "event base is not ready!");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return false;
    }

    _event_client_sptr = std::make_shared<EventFdClient>(_evfd);
    if (!_event_client_sptr)
    {
        __LOG(error, "eventfd client init fail!");
        return false;
    }

    try
    {
        //std::shared_ptr<loop_thread> this_sptr = shared_from_this();
        //using namespace std::placeholders;
        //std::function<void(evutil_socket_t, short, void *)> evcb_function = std::bind(&loop_thread::evfc_cb, this, _1, _2, _3);

        _event_server_sptr = std::make_shared<EventFdServer>(_loop_sptr->ev(), _evfd, evfc_cb, this);
        _event_server_sptr->init();
    }
    catch (std::exception &e)
    {
        __LOG(error, "!!!!!!!!!!!!exception happend when trying to create event fd server, info :" << e.what());
        return false;
    }
    __LOG(debug, "start a new loop");
    _loop_sptr->start(new_thread);
    return true;
}

void loop_thread::evfc_cb(evutil_socket_t evfd, short event, void *arg)
{
    uint64_t one;
    loop_thread *_evfd_ptr = (loop_thread *)arg;
    int ret = read(evfd, &one, sizeof one);
    if (ret != sizeof one)
    {
        __LOG(warn, "read return : " << ret);
        return;
    }
    if (_evfd_ptr)
    {
        _evfd_ptr->process_msg(one);
    }
    else
    {
        __LOG(error, "this has been deleted");
    }
}