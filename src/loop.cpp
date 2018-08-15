#include "loop.hpp"
thread_local Loop *curThreadLoop;

std::mutex Loop::_sMutex;
std::map<uint32_t, Loop *> Loop::_sLoops;
uint32_t Loop::_sIdGenerater = 0;

Loop::Loop() : _id(0),
               _base(event_base_new()),
               _thread(nullptr),
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

    if (!_base)
    {
        // note!!!! if you catch this exception
        // please remember call the distructure function
        // !!!!!!! this is important
        throw std::logic_error(CREATE_EVENT_FAIL);
    }
    evthread_make_base_notifiable(_base.get());
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
        _thread = std::make_shared<std::thread>(std::bind(&Loop::_run, this));
    }
    else
    {
        _run();
    }

    return true;
}

void Loop::wait()
{
    if (_thread && StatusFinished != _status)
    {
        _thread->join();
    }
}

void Loop::stop(bool waiting)
{
    if (StatusFinished == _status)
    {
        return;
    }

    waiting ? event_base_loopexit(_base.get(), NULL) : event_base_loopbreak(_base.get());
    onAfterStop();
}

void Loop::_run()
{
    _status = StatusRunning;
    onBeforeLoop();
    event_base_loop(_base.get(), 0);
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
