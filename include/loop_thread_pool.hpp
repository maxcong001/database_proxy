#pragma once

#include <thread>

#include <thread>
#include <chrono>
#include "util.hpp"
#include "loop_thread.hpp"

extern thread_local std::shared_ptr<loop_thread> _loop_thread_sptr;

class loop_thread_pool
{
  public:
    loop_thread_pool(unsigned int loop_num) : _loop_num(loop_num)
    {
        _select_index = 0;
    }
    ~loop_thread_pool()
    {
    }
    static loop_thread_pool *instance()
    {
        static loop_thread_pool *ins = new loop_thread_pool((std::thread::hardware_concurrency() > 1 ? std::thread::hardware_concurrency() - 1 : 1));
        return ins;
    }

    bool init()
    {
        for (unsigned int i = 0; i < _loop_num; i++)
        {
            std::shared_ptr<loop_thread> tmp_loop_thread(new loop_thread());
            add_loop(tmp_loop_thread);
            _threads.emplace_back([=]() {
                __LOG(debug, "new loop thread with ID : " << std::this_thread::get_id());
                _loop_thread_sptr = tmp_loop_thread;
                if (!tmp_loop_thread->init(false))
                {
                    __LOG(error, "start a new loop thread fail, start one again!");
                }
                else
                {
                    __LOG(error, "now set the _loop_thread_sptr");
                    
                }
                __LOG(debug, "exit thread with ID : " << std::this_thread::get_id());
            });
        }

        for (auto it : _loop_threads)
        {
            while (1)
            {
                if (!it->get_loop())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                if (it->get_loop()->status() != Loop::StatusRunning)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                break;
            }
        }

        return true;
    }
    bool stop()
    {
        for (auto it : _loop_threads)
        {
            TASK_MSG msg;
            msg.type = TASK_MSG_TYPE::EXIT_LOOP;
            msg.body = it;

            it->send2loop_thread(msg);
        }
        for (std::thread &_thread : _threads)
        {
            _thread.join();
        }
        return true;
    }
    bool send_to_all(TASK_MSG msg)
    {
        for (auto it : _loop_threads)
        {
            it->send2loop_thread(msg);
        }
        return true;
    }
    std::shared_ptr<loop_thread> get_loop_thread()
    { // to do : add lock
        _select_index++;
        std::uint32_t tmp_index = _select_index % _loop_num;
        return _loop_threads[tmp_index];
    }
    void add_loop(std::shared_ptr<loop_thread> loop_sptr)
    {
        // to do : add lock
        _loop_threads.push_back(loop_sptr);
    }

    std::vector<std::shared_ptr<loop_thread>> _loop_threads;
    std::vector<std::thread> _threads;
    
    unsigned int _loop_num;
    std::uint32_t _select_index;
};