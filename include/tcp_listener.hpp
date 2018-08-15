#pragma once

class TCPListener
{
  public:
    TCPListener() = delete;
    TCPListener(event_base *evBase_in)
    {
    }
    bool init()
    {
    }
    bool listen(std::string IP, std::uint16_t Port)
    {
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));

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

        serverAddr.sin_port = htons(port);

        _eventListener.reset(evconnlistener_new_bind(_master->ev(),
                                                     listenEventCallback, this,
                                                     LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE | LEV_OPT_THREADSAFE, -1,
                                                     (struct sockaddr *)&serverAddr, sizeof(serverAddr)));

        if (!_eventListener)
        {
            __LOG(error, "listen return fail!");
            return false;
        }

        evutil_make_socket_nonblocking(evconnlistener_get_fd(_eventListener.get()));
        evconnlistener_set_error_cb(_eventListener.get(), listenErrorCallback);
    }
    void listenErrorCallback(evconnlistener *, void *)
    {
        __LOG(error, "listen fail!");
    }
    void listenEventCallback(evconnlistener *, evutil_socket_t fd, sockaddr *remote_addr,
                             int remote_addr_len, void *arg)
    {
        /*
        in this callback function, we will dispatch fd to different worker thread.
        */


        TASK_MSG msg;
        msg.type = ADD_SESSION;
        msg.body = fd;
        loop_thread_pool::instance()->get_loop()->send2loop_thread(msg);
    }
    ListenerSPtr _eventListener;
};