#pragma once
#include <map>

/**
 * @brief on top of event_base
 * @details this class is thread safe, but event loop is sigle thread
 * if there is no event registered, event will stop auto
 */
class Loop
{
  public:
    static const std::int32_t StatusInit = 0;

    static const std::int32_t StatusRunning = 2;

    static const std::int32_t StatusFinished = 4;

    Loop();
    virtual ~Loop();

    bool init();

    /** convert to event_base * pointer*/
    inline operator event_base *() const
    {
        return _base.get();
    };

    /** @brief get event_base * pointer*/
    inline event_base *ev() const
    {
        return _base.get();
    }
    inline BaseSPtr get_base_sptr()
    {
      return _base;
    }

    inline std::uint32_t id() const
    {
        return _id;
    }


    /** get status */
    inline std::int32_t status() const
    {
        return _status;
    }

    /**
	 * @brief start event loop
	 * @details if this is called in the current thread. it will block current thread until the end of time loop
	 * see onBeforeStart
	 */
    bool start(bool newThread = true);

    void wait();

    /** if run with new thread, will stop the new thread
	 * waiting: if call the active callback before exit
	 */
    void stop(bool waiting = true);

  protected:
    virtual bool onBeforeStart();

    virtual void onBeforeLoop();

    virtual void onAfterLoop();

    virtual void onAfterStop();

  private:
    void _run();

  private:
    std::uint32_t _id;
    BaseSPtr _base;
    std::shared_ptr<std::thread> _thread_sptr;
    std::atomic<std::uint32_t> _status;

  private:
    static std::atomic<std::uint32_t> _sIdGenerater;
};