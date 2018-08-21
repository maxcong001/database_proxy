#pragma once
#include "util.hpp"

class TcpSession : public std::enable_shared_from_this<TcpSession>
{
  public:
	TcpSession() = delete;
	TcpSession(evutil_socket_t fd, BaseSPtr base_sptr) : _bev_sptr(bufferevent_socket_new(base_sptr.get(), fd, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_CLOSE_ON_FREE)), _isClosing(false)
	{
		_fd = fd;
	}
	~TcpSession()
	{
		if (!_bev_sptr)
		{
			bufferevent_free(_bev_sptr.get());
			_bev_sptr = nullptr;
		}
	}
	bool init()
	{
		if (!_bev_sptr)
		{
			__LOG(error, "init fail! evbuffer is not valid!");
			return false;
		}
		bufferevent_setcb(_bev_sptr.get(), readCallback, writeCallback, eventCallback, this);
		if (-1 == bufferevent_enable(_bev_sptr.get(), EV_READ | EV_WRITE))
		{
			bufferevent_free(_bev_sptr.get());
			_bev_sptr = nullptr;
			return false;
		}
	}

	inline evutil_socket_t get_fd() const
	{
		return _fd;
	}

	bool send(const char *data, uint32_t size)
	{
		if (!_bev_sptr || _isClosing)
		{
			return false;
		}
		return (-1 != bufferevent_write(_bev_sptr.get(), data, size));
	}
	/*
	void close(bool waiting = true)
	{
		if (!_bev_sptr)
		{
			return;
		}

		if (waiting && (evbuffer_get_length(OUTPUT_BUFFER) > 0))
		{
			_isClosing = true;
			return;
		}

		closeImpl();
	}
	void closeImpl()
	{
		_isClosing = false;
		clearInputBuffer();
		bufferevent_disable(_bev_sptr, EV_WRITE);
		shutdown(socket(), 2);
	}
	void checkClosing()
	{
		if (_isClosing && (0 == evbuffer_get_length(OUTPUT_BUFFER)))
		{
			_isClosing = false;
			closeImpl();
		}
	}
	*/

	bool drainInputBuffer(uint32_t len)
	{
		return (!_bev_sptr) ? (-1 != evbuffer_drain(INPUT_BUFFER, len)) : false;
	}

	uint32_t getInputBufferLength() const
	{
		return (!_bev_sptr) ? evbuffer_get_length(INPUT_BUFFER) : 0;
	}
	/** view input buffer */
	const uint8_t *viewInputBuffer(uint32_t size) const
	{
		return (!_bev_sptr) ? evbuffer_pullup(INPUT_BUFFER, size) : NULL;
	}
	/** read input buffer */
	bool readInputBuffer(uint8_t *dest, uint32_t size)
	{
		return (!_bev_sptr) ? (-1 != evbuffer_remove(INPUT_BUFFER, (void *)dest, size)) : false;
	}
	/** clear input buffer */
	void clearInputBuffer()
	{
		if (!_bev_sptr)
		{
			evbuffer_drain(INPUT_BUFFER, UINT32_MAX);
		}
	}

  protected:
	/**
	 * @brief 
	 * @details cal TcpServer::onSessionRead by default. 
	 * @see TcpServer::onSessionRead
	 */
	virtual void onRead();

	enum ParseResult
	{
		Completed,
		Incompleted,
		Error,
	};

  private:
	void handleEvent(short events);

	static void readCallback(struct bufferevent *bev, void *data)
	{
		TcpSession *session = (TcpSession *)data;
		session->onRead();
	}
	static void writeCallback(struct bufferevent *bev, void *data)
	{
		//TcpSession *session = (TcpSession *)data;
		//session->checkClosing();
	}
	static void eventCallback(struct bufferevent *bev, short events, void *data)
	{
		TcpSession *session = (TcpSession *)data;
		session->handleEvent(events);
	}

  private:
	evutil_socket_t _fd;
	BufferEventSPtr _bev_sptr;
	std::atomic<bool> _isClosing;
	std::uint32_t _sIdGenerater;
};
typedef std::shared_ptr<TcpSession> TcpSessionSPtr;
