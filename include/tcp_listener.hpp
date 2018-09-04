#pragma once
#include "util.hpp"
class TCPListener
{
  public:
    TCPListener() = delete;
    TCPListener(BaseSPtr base_sptr)
    {
        _base_sptr = base_sptr;
    }
    bool init()
    {
        return true;
    }
    bool listen(std::string IP, std::uint16_t Port)
    {
        __LOG(debug, "listen on IP: " << IP << ", port is : " << Port);
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        using boost::asio::ip::address;
        address addr(address::from_string(IP));
        int listenfd;
        if (addr.is_v4())
        {
            serverAddr.sin_family = AF_INET;
            evutil_inet_pton(AF_INET, IP.c_str(), &serverAddr.sin_addr);
            listenfd = socket(AF_INET, SOCK_STREAM, 0);
        }
        else if (addr.is_v6())
        {
            serverAddr.sin_family = AF_INET6;
            evutil_inet_pton(AF_INET6, IP.c_str(), &serverAddr.sin_addr);
            listenfd = socket(AF_INET6, SOCK_STREAM, 0);
        }
        else
        {
            __LOG(error, "IP info is not valid, please check. IP info is : " << IP);
            return false;
        }

        serverAddr.sin_port = htons(Port);
        if (listenfd < 0)
        {
            __LOG(error, "get fd return fail");
            return false;
        }

        int val = 1;
        /*set SO_REUSEPORT*/
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) < 0)
        {
            __LOG(error, "setsockopt() return fail!");
            return false;
        }

        int ret = bind(listenfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        if (ret != 0)
        {
            __LOG(error, "bind() return fail!");
            return false;
        }

        _eventListener.reset(evconnlistener_new(_base_sptr.get(),
                                                listenEventCallback, this,
                                                LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE | LEV_OPT_THREADSAFE, -1,
                                                listenfd),
                             evconnlistener_free);

        if (!_eventListener)
        {
            __LOG(error, "listen return fail!");
            return false;
        }

        evutil_make_socket_nonblocking(evconnlistener_get_fd(_eventListener.get()));
        evconnlistener_set_error_cb(_eventListener.get(), listenErrorCallback);
        return true;
    }
    static void listenErrorCallback(evconnlistener *, void *);
    static void listenEventCallback(evconnlistener *, evutil_socket_t fd, sockaddr *remote_addr,
                                    int remote_addr_len, void *arg);
    std::shared_ptr<evconnlistener> _eventListener;
    BaseSPtr _base_sptr;
};
