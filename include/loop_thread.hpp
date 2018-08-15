#pragma once
#if __cplusplus >= 201703L
// use std::any
#include <any>
#define TASK_ANY std::any
#define TASK_ANY_CAST std::any_cast
#else
#include <boost/any.hpp>
#define TASK_ANY boost::any
#define TASK_ANY_CAST boost::any_cast
#endif
enum CONN_TYPE : std::uint32_t
{
    IP_V4 = 0,
    IP_V6,
    UNIX_SOCKET
};
struct CONN_INFO
{
    CONN_TYPE type;
    std::string IP;
    std::uint16_t port;
    std::string source_IP;
    std::string path;
};

enum class TASK_MSG_TYPE : std::uint32_t
{
    NEW_SESSION = 0,
    DEL_SESSION,
    EXIT_LOOP,
    ADD_CONN,
    DEL_CONN,
    TASK_MSG_TYPE_MAX
};
struct TASK_MSG
{
    TASK_MSG_TYPE type;
    TASK_ANY body;
};
typedef std::queue<TASK_MSG> TASK_QUEUE;

class loop_thread : public std::enable_shared_from_this<loop_thread>
{

  public:
    loop_thread() : _loop_sptr(nullptr)
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
        loop_thread::_fd_to_session_map.clear();
        return true;
    }
    void process_msg(uint64_t num);

    bool init(bool new_thread)
    {
        _loop_sptr = std::make_shared<Loop>();
        if (！_loop_ptr)
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
        // wait until event base is ready
        // at some rainy-day case, event base maybe NULL
        if (!_event_base)
        {
            __LOG(warn, "event base is not ready!");
            return false;
        }

        _event_client_sptr = std::make_shared<EVFDClient>(_evfd);
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

    int _evfd;
    std::shared_ptr<Loop> _loop_sptr;
    std::shared_ptr<EventFdServer> _event_server_sptr;
    std::shared_ptr<EventFdClient> _event_client_sptr;
    TASK_QUEUE _task_queue;
    std::mutex mtx;

    static std::thread_local std::shared_ptr<loop_thread> _loop_thread_sptr;
    static std::thread_local std::map<evutil_socket_t, std::shared_ptr<TcpSession>> _fd_to_session_map;
    static std::thread_local std::vector<ContextSptr> _tcp_conn;
};