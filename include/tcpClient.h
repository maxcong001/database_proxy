#pragma once
/*
 * Copyright (c) 2016-20017 Max Cong <savagecm@qq.com>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tcp_socket.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include "util.hpp"

#include "loop.hpp"
#include "resp_parser.hpp"

class TcpClient : public TcpSocket
{
  public:
	TcpClient(std::shared_ptr<Loop> loop);
	virtual ~TcpClient()
	{
		__LOG(warn, "dtor of TcpClient is called, this is : " << (void *)this);
	}

	bool init()
	{
		__LOG(debug, "[TcpClient::init], this is : " << (void *)this);

		_dscp = 0;
		return true;
	}

	inline bool isConnected() const
	{
		return _isConnected; //.load(std::memory_order_acquire 	);
	}
	bool disConnect()
	{
		if (this->_bev_sptr)
		{
			this->_bev_sptr = nullptr;
		}
		return true;
	}

	bool _connect(struct sockaddr *addr, unsigned int addr_len, int fd = -1)
	{
		if (fd < 0)
		{
			__LOG(error, " fd is not valid");
			return false;
		}
		evutil_make_socket_nonblocking(fd);
		// note: note no cross check for source IP
		set_source_addr(get_conn_info().get_source_ip(), fd);

		this->_bev_sptr.reset(bufferevent_socket_new(_loop->ev(), fd,
													 BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS),
							  bufferevent_free);

		if (!this->_bev_sptr)
		{
			__LOG(error, "buffer event new return fail!");
			return false;
		}

		__LOG(debug, "[_connect]buffer_event is : " << (void *)(this->_bev_sptr.get()) << ", this is : " << (void *)this);
		if (-1 == bufferevent_socket_connect(this->_bev_sptr.get(), (struct sockaddr *)addr, addr_len))
		{
			__LOG(error, "bufferevent_socket_connect return fail!");
			this->_bev_sptr = nullptr;
			return false;
		}

		__LOG(debug, "[init] buffer_event is : " << (void *)(this->_bev_sptr.get()) << ", this is : " << (void *)this);

		bufferevent_setcb(this->_bev_sptr.get(), readCallback, writeCallback, eventCallback, this);
		if (-1 == bufferevent_enable(this->_bev_sptr.get(), EV_READ | EV_WRITE))
		{
			__LOG(error, "buffer event enable return fail");
			this->_bev_sptr = nullptr;
			return false;
		}

		int sockret = set_sockopt_tos(get_conn_info().get_dscp(), fd);
		if (sockret != 0)
		{
			__LOG(error, "Failed to set IP_TOS: DSCP" << _dscp << ", fd= " << fd << ", ret" << sockret);
		}

		return true;
	}
	void set_dscp(unsigned int dscp)
	{
		_dscp = dscp;
	}

	void set_conn_info(CONN_INFO info)
	{
		_conn_info = info;
	}
	CONN_INFO get_conn_info()
	{
		return _conn_info;
	}
	CONN_INFO _conn_info;

	void reconnect()
	{
		if (this->_bev_sptr)
		{
			this->_bev_sptr = nullptr;
		}
		connect(get_conn_info());
	}
	bool connect(CONN_INFO _info)
	{
		set_conn_info(_info);
		switch (_info.get_conn_type())
		{
		case CONN_TYPE::IP_V4:
		{
			int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
			__LOG(debug, "connect IPv4 using socket fd : " << sock_fd);
			if (sock_fd < 0)
			{
				__LOG(error, " create a socket fd return fail");
				return false;
			}
			uint16_t port = 0;
			try
			{
				// note: convert int to uint16
				port = std::stoi(_info.get_port());
			}
			catch (std::exception &e)
			{
				__LOG(error, "!!!!!!!!!!!!exception happend when trying to cast port, info :" << e.what() << ". string is : " << _info.get_port());
				return false;
			}
			struct sockaddr_in serverAddr;
			memset(&serverAddr, 0, sizeof(serverAddr));
			serverAddr.sin_family = AF_INET;
			evutil_inet_pton(AF_INET, _info.get_ip().c_str(), &serverAddr.sin_addr);
			serverAddr.sin_port = htons(port);
			return _connect((struct sockaddr *)&serverAddr, sizeof(serverAddr), sock_fd);
		}
		break;
		case CONN_TYPE::IP_V6:
		{

			int sock_fd = socket(AF_INET6, SOCK_STREAM, 0);
			__LOG(debug, "connect IPv6 using socket fd : " << sock_fd);
			if (sock_fd < 0)
			{
				__LOG(error, " create a socket fd return fail");
				return false;
			}
			uint16_t port = 0;
			try
			{
				// note: convert int to uint16
				port = std::stoi(_info.get_port());
			}
			catch (std::exception &e)
			{
				__LOG(error, "!!!!!!!!!!!!exception happend when trying to cast port, info :" << e.what());
				return false;
			}
			struct sockaddr_in6 serverAddr;
			memset(&serverAddr, 0, sizeof(serverAddr));
			serverAddr.sin6_port = htons(port);
			serverAddr.sin6_family = AF_INET6;
			evutil_inet_pton(AF_INET6, _info.get_ip().c_str(), &serverAddr.sin6_addr);
			return _connect((struct sockaddr *)&serverAddr, sizeof(serverAddr), sock_fd);
		}
		break;
		case CONN_TYPE::UNIX_SOCKET:
		{
			int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
			__LOG(debug, "connect using unix socket fd : " << sock_fd);
			if (sock_fd < 0)
			{
				__LOG(error, " create a socket fd return fail");
				return false;
			}

			struct sockaddr_un serverAddr;
			memset(&serverAddr, 0, sizeof(serverAddr));
			serverAddr.sun_family = AF_UNIX;
			strncpy(serverAddr.sun_path, _info.get_path().c_str(), _info.get_path().size());
			return _connect((struct sockaddr *)&serverAddr, sizeof(serverAddr), sock_fd);
		}

		break;
		default:
			__LOG(error, "not support type! type is : " << (int)_info.get_conn_type());
			return false;
			break;
		}
	}

	// should be called before connect
	bool set_source_addr(std::string source_addr, int fd = -1)
	{
		if (source_addr.empty())
		{
			__LOG(debug, "no source IP, no need to set ");
			return true;
		}
		evutil_socket_t s;
		if (fd < 0)
		{
			s = get_socket();
		}
		else
		{
			s = fd;
		}

		if (s < 0)
		{
			__LOG(error, "invalid socket fd!");
			return false;
		}
		struct addrinfo *bservinfo, *b; //, hints ;
		if (!source_addr.empty())
		{
			int rv = -1;
			/* Using getaddrinfo saves us from self-determining IP_V4 vs IP_V6 */
			if ((rv = getaddrinfo(source_addr.c_str(), NULL, NULL, &bservinfo)) != 0) //&hints, &bservinfo)) != 0)
			{
				__LOG(error, "get addr info return fail!");
				// fail, need to call freeaddrinfo? Seems not needed
				//freeaddrinfo(bservinfo);
				return false;
			}
			for (b = bservinfo; b != NULL; b = b->ai_next)
			{
				if (bind(s, b->ai_addr, b->ai_addrlen) != -1)
				{
					__LOG(error, "bind faill!!!");
					break;
				}
			}
		}
		else
		{
			return false;
		}
		freeaddrinfo(bservinfo);
		return true;
	}

	// not : this should call before connect
	int set_sockopt_tos(unsigned int dscp, int _fd = -1)
	{
		__LOG(debug, "[set_socket_option] fd is :" << _fd);
		if (!this->_bev_sptr)
		{
			return -1;
		}

		int ret;

		struct sockaddr_storage ss;
		socklen_t sslen = sizeof(ss);
		evutil_socket_t fd = _fd;
		if (fd < 0)
		{
			fd = get_socket();
		}

		if (fd < 0)
		{
			__LOG(error, "invalid fd!");
			return -1;
		}

		ret = getsockname(fd, (struct sockaddr *)&ss, &sslen);
		if (ret != 0)
		{
			return ret;
		}

		unsigned char tos = (unsigned char)((dscp & 0x3f) << 2);

		switch (((struct sockaddr *)&ss)->sa_family)
		{
		case AF_INET:
		{
			ret = setsockopt(fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));
			__LOG(debug, "set socket option return : " << ret);
		}
		break;
		case AF_INET6:
		{
			ret = setsockopt(fd, IPPROTO_IPV6, IPV6_TCLASS, &tos, sizeof(tos));
		}
		break;
		default:
			return -1;
		}
		return ret;
	}

	std::shared_ptr<Loop> get_loop()
	{
		return _loop;
	}

	virtual void onRead()
	{
		uint32_t length = this->getInputBufferLength();

		if (length <= 0)
		{
			__LOG(debug, "read : " << length << " byte");
			return;
		}

		const uint8_t *buf = this->viewInputBuffer(length);
		if (!buf)
		{
			__LOG(debug, "buf is not valid, ptr is : " << (void *)buf);
			return;
		}

		__LOG(debug, "data in the buf is : " << std::string((char *)(const_cast<uint8_t *>(buf)), length));

		auto ret = rasp_parser::process_resp((char *)(const_cast<uint8_t *>(buf)), length, [this](char *buf, size_t buf_length) {
			__LOG(debug, "now there is a RESP message for TCP client");
		});

		if (std::get<0>(ret) > 0 && !this->drainInputBuffer(std::get<0>(ret)))
		{
			__LOG(error, "drain input buffer fail!");
			return;
		}
	};

	virtual void onDisconnected()
	{
		__LOG(debug, "dis connected");
	};

	virtual void onConnected(int error)
	{
		__LOG(debug, "connected");
	};

  private:
	void handleEvent(short events);

	static void readCallback(struct bufferevent *bev, void *ctx);
	static void writeCallback(struct bufferevent *bev, void *ctx);
	static void eventCallback(struct bufferevent *bev, short events, void *ctx);

  private:
	std::shared_ptr<Loop> _loop;
	std::atomic<bool> _isConnected;
	unsigned int _dscp;
};
