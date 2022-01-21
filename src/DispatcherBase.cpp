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
        if (m_Thread.joinable())
        {
            m_Thread.join();
        }
        return true;
    }

    void
    DispatcherBase::KeepAliveInternal(void)
    {
        std::unique_lock<std::mutex> lk(m_ConditionMutex);
        //
        // We will only ever need to wake if a task
        // is posted to the cross thread queue as
        // m_Queue can only be posted to by the current
        // thread (which is waiting...)
        //
        m_TaskAvailable.wait(lk, [this]{
            return m_CrossThread.size() > 0 /*|| m_Queue.size() > 0*/;});
    }

    void
    DispatcherBase::DispatchLoop(void)
    {
        ThreadQueue = this;
        m_ThreadId = std::this_thread::get_id();

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

        //
        // Notify the callback
        //
        if (m_DestructionHandler)
        {
            m_DestructionHandler(this);
        }
    }

    DispatcherBase::~DispatcherBase(void)
    {
        //
        // Check if someone is already waiting on us
        // if not, we can join ourself to cleanup the thread
        //
        Wait();

#ifdef DEBUG
        std::cerr << "Dispatcher \"" << GetName() << "\" terminating" << std::endl;
#endif
    }

    void
    DispatcherBase::Run(void)
    /*++
      Start dispatcher on a new thread
    --*/
    {
        m_Thread = std::thread(
            std::bind(&DispatcherBase::DispatchLoop, this)
        );
    }

    void
    DispatcherBase::Enter(void)
    /*++
      Enter the dispatcher on the current thread
    --*/
    {
        DispatchLoop();
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
        if (std::this_thread::get_id() == m_ThreadId)
        {
            m_Queue.push_back(std::move(TaskJob));
            m_ReceivedTask = true;
        }
        else
        {
            std::lock_guard<std::mutex> guard(m_CrossThreadMutex);
            std::unique_lock<std::mutex> lk(m_ConditionMutex);
            m_CrossThread.push_back(std::move(TaskJob));
            lk.unlock();
            m_TaskAvailable.notify_one();
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
        assert(std::this_thread::get_id() != m_ThreadId);
        
        auto reply = Job(std::move(Reply), Priority, ThreadQueue);
        auto job = Job(std::move(Task), Priority, this, reply);
        PostTask(job);
    }

}