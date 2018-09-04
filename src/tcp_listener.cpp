#include "tcp_listener.hpp"
#include "loop_thread_pool.hpp"
extern thread_local std::shared_ptr<loop_thread> _loop_thread_sptr;
void TCPListener::listenErrorCallback(evconnlistener *, void *)
{
    __LOG(error, "listen fail!");
}
void TCPListener::listenEventCallback(evconnlistener *, evutil_socket_t fd, sockaddr *remote_addr,
                                      int remote_addr_len, void *arg)
{
    evutil_make_socket_nonblocking(fd);
    __LOG(error, "now try to add a new session, this thread is :" << std::this_thread::get_id() << ", socket fd is :" << fd);
    std::shared_ptr<TcpSession> _session_sptr(new TcpSession(fd, _loop_thread_sptr->get_loop()->get_base_sptr()));
    _session_sptr->init();
    _loop_thread_sptr->_fd_to_session_map[fd] = _session_sptr;
}