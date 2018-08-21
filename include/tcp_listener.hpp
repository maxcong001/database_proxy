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
        if (addr.is_v4())
        {
            serverAddr.sin_family = AF_INET;
            evutil_inet_pton(AF_INET, IP.c_str(), &serverAddr.sin_addr);
        }
        else if (addr.is_v6())
        {
            serverAddr.sin_family = AF_INET6;
            evutil_inet_pton(AF_INET6, IP.c_str(), &serverAddr.sin_addr);
        }
        else
        {
            __LOG(error, "IP info is not valid, please check. IP info is : " << IP);
            return false;
        }

        serverAddr.sin_port = htons(Port);

        _eventListener.reset(evconnlistener_new_bind(_base_sptr.get(),
                                                     listenEventCallback, this,
                                                     LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE | LEV_OPT_THREADSAFE, -1,
                                                     (struct sockaddr *)&serverAddr, sizeof(serverAddr)),
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
