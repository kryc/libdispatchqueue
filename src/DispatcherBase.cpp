#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <mutex>
#include <algorithm>
#include <deque>

#include "DispatcherBase.hpp"

namespace dispatch
{
    extern thread_local DispatcherBase* ThreadQueue;

    DispatcherBase::DispatcherBase(Callable& Job)
    {
        PostTask(Job);
    }

    void DispatcherBase::Wait(void)
    {
        m_Thread.join();
    }

    void
    DispatcherBase::DispatchLoop(void)
    {
        ThreadQueue = this;
        for (;;)
        {
            if (m_Queue.size() == 0)
            {
                if (m_Stop == true)
                {
                    break;
                }
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(200ms);
                continue;
            }

            auto callable = std::move(m_Queue.front());
            m_Queue.pop_front();
            callable();
        }

        //
        // This will be the last thing that the
        // std::thread will do before completing and
        // being joinable
        //
        m_Completed = true;
    }

    DispatcherBase::~DispatcherBase(void)
    {
        m_Thread.join();
        std::cerr << "Dispatcher terminating" << std::endl;
    }

    void
    DispatcherBase::Run(void)
    {
        m_Thread = std::thread(
            std::bind(&DispatcherBase::DispatchLoop, this)
        );
    }

    void
    DispatcherBase::Stop(void)
    {
        m_Stop = true;
    }

    void
    DispatcherBase::PostTask(const Callable& Entrypoint)
    {
        auto job = Job(std::move(Entrypoint), PRIORITY_NORMAL, this);
        m_Queue.push_back(std::move(job));
    }

}