#include "tcp_listener.hpp"
#include "loop_thread_pool.hpp"
void TCPListener::listenErrorCallback(evconnlistener *, void *)
{
    __LOG(error, "listen fail!");
}
void TCPListener::listenEventCallback(evconnlistener *, evutil_socket_t fd, sockaddr *remote_addr,
                                      int remote_addr_len, void *arg)
{
    /*
        in this callback function, we will dispatch fd to different worker thread.
        */

    TASK_MSG msg;
    msg.type = TASK_MSG_TYPE::NEW_SESSION;
    msg.body = fd;
    loop_thread_pool::instance()->get_loop()->send2loop_thread(msg);
}