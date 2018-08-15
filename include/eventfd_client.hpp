#pragma once
#include "logger.hpp"
#include <unistd.h>
#include <sys/eventfd.h>
/**
 * @brief EventFdClient base 
 */
class EventFdClient
{
  public:
    EventFdClient() = delete;
    // Note:!! please make sure your fd is non-blocking
    // for example: std::int32_t ev_fd = eventfd(0, EFD_NONBLOCK|EFD_CLOEXEC);
    EventFdClient(std::int32_t efd) : one(1)
    {
        eventFd = efd;
        __LOG(debug, "event fd is " << eventFd);
    }
    virtual ~EventFdClient()
    {
        // note: we will not destroy eventfd here, app should do the destroy work after eventfd client and server exit
    }
    bool init()
    {
        return true;
    }

    bool send()
    {
        std::int32_t ret = write(eventFd, &one, sizeof(one));
        if (ret != sizeof(one))
        {
            __LOG(error, "write event fd : " << eventFd << " fail");
            return false;
        }
        return true;
    }

  private:
    std::int32_t eventFd;
    uint64_t one;
};