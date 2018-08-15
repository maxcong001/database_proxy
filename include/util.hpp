#pragma once

template <class T, void (*deleter)(T *)>
class CSmartPtr : public std::shared_ptr<T, void (*)(T *)>
{
  public:
    CSmartPtr() : std::shared_ptr<T, void (*)(T *)>(nullptr, deleter) {}
    CSmartPtr(T *object) : std::shared_ptr<T, void (*)(T *)>(object, deleter) {}
};

typedef CSmartPtr<event_base, event_base_free> BaseSPtr;
typedef CSmartPtr<evbuffer, evbuffer_free> BufferSPtr;
typedef CSmartPtr<bufferevent, bufferevent_free> BufferEventSPtr;
typedef CSmartPtr<evconnlistener, evconnlistener_free> ListenerSPtr;

typedef CSmartPtr<redisAsyncContext, redisAsyncFree> ContextSptr;