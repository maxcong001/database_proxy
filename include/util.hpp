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
#include <string>
#include <boost/lexical_cast.hpp>
#include "boost/asio/ip/address.hpp"
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
#define INPUT_BUFFER bufferevent_get_input(_bev_sptr.get())
#define OUTPUT_BUFFER bufferevent_get_output(_bev_sptr.get())
enum CONN_TYPE : std::uint32_t
{
	IP_V4 = 0,
	IP_V6,
	UNIX_SOCKET,
	UN_KNOWN
};
struct CONN_INFO
{
	CONN_TYPE type;
	std::string IP;
	std::uint16_t port;
	std::uint16_t source_port;
	std::string source_IP;
	std::string path;
	unsigned int dscp;

	CONN_INFO()
	{
		type = CONN_TYPE::UN_KNOWN;
		dscp = 0;
	}

	CONN_TYPE get_conn_type()
	{
		return type;
	}

	std::tuple<std::string, std::string, std::string, std::string, bool> get_ip_port()
	{
		bool isV6 = ((type == CONN_TYPE::IP_V6) ? true : false);
		return std::make_tuple(IP, get_port(), source_IP, get_source_port(), isV6);
	}
	std::string get_ip()
	{
		return IP;
	}
	std::string get_port()
	{
		std::string ret_port;
		try
		{
			ret_port = std::to_string(port);
		}
		catch (std::bad_alloc &e)
		{
			__LOG(error, "!!!!!!!!!!!!exception happend when trying to cast port, info :" << e.what());
		}
		return ret_port;
	}
	std::string get_source_port()
	{
		std::string ret_port;
		try
		{
			ret_port = std::to_string(source_port);
		}
		catch (std::bad_alloc &e)
		{
			__LOG(error, "!!!!!!!!!!!!exception happend when trying to cast port, info :" << e.what());
		}
		return ret_port;
	}
	std::string get_source_ip()
	{
		return source_IP;
	}
	std::string get_path()
	{
		return path;
	}
	unsigned int get_dscp()
	{
		return dscp;
	}
	/*
    inline bool operator<(const CONN_INFO &rhs)
    {
        return true;
    }
    */
	inline CONN_INFO &operator=(CONN_INFO rhs)
	{
		type = rhs.type;
		IP = rhs.IP;
		port = rhs.port;
		source_IP = rhs.source_IP;
		source_port = rhs.source_port;
		path = rhs.path;
		dscp = rhs.dscp;
		return *this;
	}
	inline bool operator==(CONN_INFO rhs)
	{
		return (source_port == rhs.source_port) && (dscp == rhs.dscp) && ((type == rhs.type) && (!IP.compare(rhs.get_ip())) && (port == rhs.port) && (!source_IP.compare(rhs.get_source_ip())) && (!path.compare(rhs.get_path())));
	}
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

typedef uint64_t SessionId;
const SessionId SESSION_ID_INVALID = 0;

typedef evutil_socket_t SocketFd;
const SocketFd SOCKET_FD_INVALID = -1;