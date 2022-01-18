#include <algorithm>
#include <assert.h>
#include <iterator>
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

    DispatcherBase::DispatcherBase(
        const Callable& Entrypoint
    )
    {
        PostTask(Entrypoint);
    }

    DispatcherBase::DispatcherBase(
        const std::string& Name,
        const Callable& Entrypoint
    )
    {
        m_Name = Name;
        PostTask(Entrypoint);
    }

    bool
    DispatcherBase::Wait(
        void
    )
    {
        if (m_Waiting)
        {
            return false;
        }

        m_Waiting = true;
        m_Thread.join();
        return true;
    }

    void
    DispatcherBase::KeepAliveInternal(void)
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(200ms);
    }

    void
    DispatcherBase::DispatchLoop(void)
    {
        ThreadQueue = this;
        for (;;)
        {
            if (m_CrossThread.size() > 0)
            {
                std::lock_guard<std::mutex> guard(m_CrossThreadMutex);
                for (auto& callable : m_CrossThread)
                {
                    PostTaskInternal(std::move(callable));
                }
                m_CrossThread.clear();
            }

            if (m_Queue.size() == 0)
            {
                //
                // Check if we kill this dispatcher
                //
                if (m_Stop == true ||
                    (m_KeepAlive == false && m_ReceivedTask == true))
                {
                    break;
                }
                
                //
                // Push the keep alive task onto the queue
                //
                PostTaskInternal(
                    std::bind(&DispatcherBase::KeepAliveInternal, this)
                );
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
        //
        // Check if someone is already waiting on us
        // if not, we can join ourself to cleanup the thread
        //
        this->Wait();

#ifdef DEBUG
        std::cerr << "Dispatcher \"" << GetName() << "\" terminating" << std::endl;
#endif
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
    DispatcherBase::PostTaskInternal(
        const Callable& Entrypoint
    )
    {
        auto job = Job(std::move(Entrypoint), PRIORITY_NORMAL, this);
        m_Queue.push_back(std::move(job));
        m_ReceivedTask = true;
    }

    void
    DispatcherBase::PostTask(const Callable& Entrypoint)
    {
        //
        // If we are currently on this dispatcher's thread
        // then we can safely just append this job to our
        // own queue. If not we need to take the mutex and
        // append it to the wait queue
        //
        if (std::this_thread::get_id() == m_Thread.get_id())
        {
            PostTaskInternal(Entrypoint);
        }
        else
        {
            std::lock_guard<std::mutex> guard(m_CrossThreadMutex);
            m_CrossThread.push_back(std::move(Entrypoint));
        }
    }

}