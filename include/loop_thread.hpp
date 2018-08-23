#pragma once
#include "util.hpp"
#include <thread>
#include <chrono>

#include "loop.hpp"
#include "eventfd_client.hpp"
#include "eventfd_server.hpp"
#include "tcp_session.hpp"
#include "redis_tcp_client.hpp"
class loop_thread : public std::enable_shared_from_this<loop_thread>
{
  public:
    loop_thread()
    {
        _evfd = 0;
    }
    ~loop_thread()
    {
        stop();
        __LOG(warn, "loop thread is exiting!");
    }
    bool stop()
    {
        if (!_evfd)
        {
            __LOG(warn, "event fd is 0");
            return false;
        }
        else
        {
            close(_evfd);
            _evfd = 0;
        }
        _loop_sptr->stop(true);

        return true;
    }
    void process_msg(uint64_t num);

    bool init(bool new_thread);
    // thread safe
    bool send2loop_thread(TASK_MSG msg)
    {
        std::lock_guard<std::mutex> lck(mtx);
        _task_queue.push(msg);
        if (!_evfd)
        {
            __LOG(error, "event fd is not ready");
            return false;
        }
        _event_client_sptr->send();
        return true;
    }
    std::shared_ptr<Loop> get_loop()
    {
        return _loop_sptr;
    }
    static void evfc_cb(evutil_socket_t evfd, short event, void *arg);

    int _evfd;
    std::shared_ptr<Loop> _loop_sptr;
    std::shared_ptr<EventFdServer> _event_server_sptr;
    std::shared_ptr<EventFdClient> _event_client_sptr;
    TASK_QUEUE _task_queue;
    std::mutex mtx;

    
    std::map<evutil_socket_t, std::shared_ptr<TcpSession>> _fd_to_session_map;
    std::vector<std::shared_ptr<redis_tcp_client>> _connection_sptr_vector;
};