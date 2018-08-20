#include "loop_thread.hpp"

#include "tcpClient.h"
thread_local std::shared_ptr<loop_thread> loop_thread::_loop_thread_sptr;
thread_local std::map<evutil_socket_t, std::shared_ptr<TcpSession>> loop_thread::_fd_to_session_map;
thread_local std::vector<std::shared_ptr<TcpClient>> loop_thread::_connection_sptr_vector;

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
        case TASK_MSG_TYPE::NEW_SESSION:
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
        case TASK_MSG_TYPE::NEW_LISTENER:
        {
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
            auto listener_ptr = new TCPListener(_loop_sptr->get_base_sptr());
            listener_ptr->listen(conn.IP, conn.type);
        }
        break;
        case TASK_MSG_TYPE::DEL_SESSION:
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
    __LOG(debug, "init task with ID :" << _evfd);
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
        auto this_sptr = std::shared_from_this();
        _event_server_sptr = std::make_shared<EventFdServer>(_loop_sptr->ev(), _evfd, [this_sptr](int fd, short event, void *args) {
            uint64_t one;
            auto keep_this_sptr = this_sptr;
            int ret = read(fd, &one, sizeof one);
            if (ret != sizeof one)
            {
                __LOG(warn, "read return : " << ret);
                return;
            }
            if (keep_this_sptr)
            {
                keep_this_sptr->process_msg(one);
            }
            else
            {
                __LOG(error, "this has been deleted");
            }
        },
                                                             this);
    }
    catch (std::exception &e)
    {
        __LOG(error, "!!!!!!!!!!!!exception happend when trying to create event fd server, info :" << e.what());
        return false;
    }
    _loop_sptr->start(new_thread);
    return true;
}