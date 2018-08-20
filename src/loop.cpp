#include "loop.hpp"
//thread_local std::shared_ptr<Loop> curThreadLoop_sptr;
std::atomic<std::uint32_t> Loop::_sIdGenerater;

Loop::Loop() : _id(0),
               _base_sptr(event_base_new()),
               _thread_sptr(nullptr),
               _status(StatusInit)
{
}

Loop::~Loop()
{
    stop();
}
bool Loop::init()
{
    _id = _sIdGenerater++;
    evthread_use_pthreads();

    if (!_base_sptr)
    {
        // note!!!! if you catch this exception
        // please remember call the distructure function
        // !!!!!!! this is important
        throw std::logic_error(CREATE_EVENT_FAIL);
    }
    evthread_make_base_notifiable(_base_sptr.get());
    return true;
}
bool Loop::start(bool newThread)
{
    if (_status != StatusInit)
    {
        __LOG(warn, "Loop has already started!");
        return false;
    }

    if (!onBeforeStart())
    {
        __LOG(error, "before start function return false");
        return false;
    }

    if (newThread)
    {
        _thread_sptr = std::make_shared<std::thread>(std::bind(&Loop::_run, this));
    }
    else
    {
        _run();
    }

    return true;
}

void Loop::wait()
{
    if (_thread_sptr && StatusFinished != _status)
    {
        _thread_sptr->join();
    }
}

void Loop::stop(bool waiting)
{
    if (StatusFinished == _status)
    {
        return;
    }

    waiting ? event_base_loopexit(_base_sptr.get(), NULL) : event_base_loopbreak(_base_sptr.get());
    onAfterStop();
}

void Loop::_run()
{
    _status = StatusRunning;
    onBeforeLoop();
    event_base_loop(_base_sptr.get(), 0);
    onAfterLoop();
    _status = StatusFinished;
}

bool Loop::onBeforeStart()
{
    return true;
}

void Loop::onBeforeLoop()
{
}

void Loop::onAfterLoop()
{
}

void Loop::onAfterStop()
{
}
