#include "tcp_socket.hpp"
#include <event2/buffer.h>

TcpSocket::TcpSocket() : _bev_sptr(NULL),
                         _isClosing(false)
{
}

TcpSocket::~TcpSocket()
{
}

evutil_socket_t TcpSocket::get_socket() const
{
    return (_bev_sptr) ? SOCKET_FD_INVALID : bufferevent_getfd(_bev_sptr.get());
}

void TcpSocket::getAddr(struct sockaddr_in *dest, uint32_t size) const
{
    uint32_t addrSize = sizeof(struct sockaddr_in);
    if ((!_bev_sptr) || (size < addrSize))
    {
        return;
    }
    getpeername(get_socket(), (struct sockaddr *)dest, &addrSize);
}

bool TcpSocket::send(const char *data, uint32_t size)
{
    if ((!_bev_sptr) || _isClosing)
    {
        return false;
    }
    return (-1 != bufferevent_write(_bev_sptr.get(), data, size));
}

void TcpSocket::close(bool waiting)
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

uint32_t TcpSocket::getInputBufferLength() const
{
    return (_bev_sptr) ? evbuffer_get_length(INPUT_BUFFER) : 0;
}

const uint8_t *TcpSocket::viewInputBuffer(uint32_t size) const
{
    return (_bev_sptr) ? evbuffer_pullup(INPUT_BUFFER, size) : NULL;
}

bool TcpSocket::readInputBuffer(uint8_t *dest, uint32_t size)
{
    return (_bev_sptr) ? (-1 != evbuffer_remove(INPUT_BUFFER, (void *)dest, size)) : false;
}

void TcpSocket::clearInputBuffer()
{
    if (_bev_sptr)
    {
        evbuffer_drain(INPUT_BUFFER, UINT32_MAX);
    }
}

bool TcpSocket::drainInputBuffer(uint32_t len)
{
    return (_bev_sptr) ? (-1 != evbuffer_drain(INPUT_BUFFER, len)) : false;
}

void TcpSocket::checkClosing()
{
    if (_isClosing && (0 == evbuffer_get_length(OUTPUT_BUFFER)))
    {
        _isClosing = false;
        closeImpl();
    }
}

void TcpSocket::closeImpl()
{
    _isClosing = false;
    clearInputBuffer();
    bufferevent_disable(_bev.get(), EV_WRITE);
    shutdown(get_socket(), 2);
}
