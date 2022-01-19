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
                    PostTask(callable);
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
                PostTask(
                    std::bind(&DispatcherBase::KeepAliveInternal, this)
                );
            }

            auto job = std::move(m_Queue.front());
            m_Queue.pop_front();
            job();
            if (job.HasReply())
            {
                auto reply = job.GetReply();
                auto dispatcher = (DispatcherBase*)reply->GetDispatcher();
                dispatcher->PostTask(*reply);
            }
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
    DispatcherBase::PostTask(
        Job& TaskJob
    )
    {
        if (std::this_thread::get_id() == m_Thread.get_id())
        {
            m_Queue.push_back(std::move(TaskJob));
            m_ReceivedTask = true;
        }
        else
        {
            std::lock_guard<std::mutex> guard(m_CrossThreadMutex);
            m_CrossThread.push_back(std::move(TaskJob));
        }
    }

    void
    DispatcherBase::PostTask(
        const Callable& Task,
        const TaskPriority Priority
    )
    {
        auto job = Job(std::move(Task), Priority, this);
        PostTask(job);
    }

    void
    DispatcherBase::PostTaskAndReply(
        const Callable& Task,
        const Callable& Reply,
        const TaskPriority Priority
    )
    {
        assert(std::this_thread::get_id() != m_Thread.get_id());
        
        std::lock_guard<std::mutex> guard(m_CrossThreadMutex);
        auto reply = Job(std::move(Reply), Priority, ThreadQueue);
        auto job = Job(std::move(Task), Priority, this, reply);
        m_CrossThread.push_back(std::move(job));
    }

}