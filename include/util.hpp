#pragma once
#include <event.h>

#include <event2/listener.h>
#include <event2/bufferevent.h>

#include <map>
#include <queue>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <event2/thread.h>

#include "logger.hpp"

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

#define CREATE_EVENT_FAIL "CREATE_EVENT_FAIL"
#define EVENTFD_EVENT_NEW_ERROR "EVENTFD_EVENT_NEW_ERROR"
#define EVENTFD_EVENT_ADD_ERROR "EVENTFD_EVENT_ADD_ERROR"
#define TIMERMANAGER_EVENT_NEW_ERROR "TIMERMANAGER_EVENT_NEW_ERROR"
#define TIMERMANAGER_ADD_ERROR "TIMERMANAGER_ADD_ERROR"

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

	NEW_LISTENER,
	DEL_LISTENER,

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

template <typename T, void (*deleter)(T *)>
class CSmartPtr : public std::shared_ptr<T>
{
  public:
	CSmartPtr() : std::shared_ptr<T>(nullptr, deleter) {}
	CSmartPtr(T *object) : std::shared_ptr<T>(object, deleter) {}
};

typedef CSmartPtr<event_base, event_base_free> BaseSPtr;
typedef CSmartPtr<evbuffer, evbuffer_free> BufferSPtr;
typedef CSmartPtr<bufferevent, bufferevent_free> BufferEventSPtr;
typedef CSmartPtr<evconnlistener, evconnlistener_free> ListenerSPtr;
