#pragma once
#include "logger.hpp"
#include <unistd.h>
#include <sys/eventfd.h>
#include "util.hpp"
/**
 * @brief EventFdServer base 
 */
class EventFdServer
{
  public:
    //typedef void (*evCb)(evutil_socket_t fd, short event, void *args);
    using evCb = std::function<void(evutil_socket_t, short, void *)>;
    EventFdServer() = delete;
    // Note:!! please make sure your fd is non-blocking
    // for example: std::int32_t ev_fd = eventfd(0, EFD_NONBLOCK|EFD_CLOEXEC);
    // Note:!! please make sure that you pass in a right envent base loop
    // event base is not changeable, if you want to change it, please kill this object and start a new one
    // will add some some check later.....
    EventFdServer(event_base *evBase_in, std::int32_t efd, evCb cb, void *arg) : eventFd(efd), evBase(evBase_in), eventCallback(cb), _arg(arg)
    {
    }
    virtual ~EventFdServer()
    {
        if (NULL != _event)
        {
            event_free(_event);
            _event = NULL;
        }
        // note: we will not stop event loop, it is app's job
    }

    bool init()
    {
        if (!eventCallback)
        {
            __LOG(error, "eventfd callback is not valid");
        }
        void (*const *ev_cb)(evutil_socket_t, short, void *) = eventCallback.target<void (*)(evutil_socket_t, short, void *)>();
        __LOG(warn, "function pointer is : " << (void *)eventCallback.target<void (*)(evutil_socket_t, short, void *)>());
        if (!(ev_cb))
        {
            __LOG(error, "there is no callback function set!");
            return false;
        }
        _event = event_new(evBase, eventFd, EV_READ | EV_PERSIST, *ev_cb, _arg);
        if (NULL == _event)
        {
            // note!!!! if you catch this exception
            // please remember call the distructure function
            // !!!!!!! this is important
            throw std::logic_error(EVENTFD_EVENT_NEW_ERROR);
        }

        __LOG(debug, "add eventfd event to loop");
        if (0 != event_add(_event, NULL))
        {
            // note!!!! if you catch this exception
            // please remember call the distructure function
            // !!!!!!! this is important
            throw std::logic_error(EVENTFD_EVENT_ADD_ERROR);
        }
        return true;
    }
    void set_evfd_arg(void *arg)
    {
        _arg = arg;
    }

    /** convert to event_base * pointer*/
    inline operator event_base *() const
    {
        return evBase;
    };

  private:
    std::int32_t eventFd;
    event_base *evBase;
    evCb eventCallback;
    struct event *_event;
    void *_arg;
};
