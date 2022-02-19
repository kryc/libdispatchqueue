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
#include "Job.hpp"

namespace dispatch
{
    extern thread_local DispatcherBase* ThreadQueue;
    extern thread_local DispatcherBase* ThreadDispatcher;

    const dispatch::timepoint MAXTIME = std::chrono::system_clock::now() + std::chrono::hours(24*365);

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
        std::unique_lock<std::mutex> lk(m_CrossThreadMutex);
        //
        // We will only ever need to wake if a task
        // is posted to the cross thread queue as
        // m_Queue can only be posted to by the current
        // thread (which is waiting...)
        //
        if (m_DelayedTasks == 0)
        {
            if (m_Queue.size() > 0)
            {
                return;
            }
            // assert(m_Queue.size() == 0);
            m_TaskAvailable.wait(lk, [this]{
                return m_CrossThread.size() > 0;}
            );
        }
        else
        {
            m_TaskAvailable.wait_until(lk, m_NextDelayedTask, [this]{
                return m_CrossThread.size() > 0;}
            );
        }
    }

    void
    DispatcherBase::SetThreadDispatcher(
        DispatcherBase* Dispatcher
    )
    {
        m_ThreadDispatcher = Dispatcher;
    }

    void
    DispatcherBase::DispatchJob(
        Job ToRun
    )
    {
        if (ToRun.IsDelayed())
        {
            m_DelayedTasks--;
        }
        assert(ToRun.ShouldRunNow());
        ToRun();
        if (ToRun.HasReply())
        {
            auto reply = std::move(*ToRun.GetReply());
            auto dispatcher = (DispatcherBase*)reply.GetDispatcher();
            dispatcher->PostTaskInternal(std::move(reply));
        }
        m_TasksCompleted++;
    }

    void
    DispatcherBase::DispatchLoop(void)
    {
        assert(ThreadQueue == nullptr);
        ThreadQueue = this;
        if (m_ThreadDispatcher != nullptr)
        {
            ThreadDispatcher = m_ThreadDispatcher;
        }
        m_ThreadId = std::this_thread::get_id();

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

            if (m_Queue.size() == 0 || m_Queue.size() == m_DelayedTasks)
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
                // Notify the completion handler only
                // if we have no delayed tasks
                //
                if (m_Queue.size() == 0)
                {
                    NotifyCompletion();
                }

                //
                // We need to find the next delayed task
                //
                if (m_DelayedTasks != 0)
                {
                    m_NextDelayedTask = dispatch::MAXTIME;
                    for (auto & job : m_Queue)
                    {
                        if (job.IsDelayed())
                        {
                            auto time = job.GetDispatchTime();
                            if (time < m_NextDelayedTask)
                            {
                                m_NextDelayedTask = time;
                            }
                        }
                    }
                }
                
                //
                // Push the keep alive task onto the queue
                //
                auto job = Job(dispatch::bind(&DispatcherBase::KeepAliveInternal, this), TaskPriority::PRIORITY_NORMAL, this);
                this->PostTaskInternal(
                    std::move(job)
                );
                m_Keepalives++;
            }

            auto job = std::move(m_Queue.front());
            m_Queue.pop_front();

            if (job.ShouldRunNow())
            {
                DispatchJob(std::move(job));
            }
            else if (job.IsDelayed())
            {
                //
                // Delayed task but not to run now
                // Push it back onto the queue
                //
                m_DelayedTasks--;
                this->PostTaskInternal(
                    std::move(job)
                );
            }
            else
            {
                assert(false);
            }
        }

        //
        // We are no longer the dispatcher for this thread
        //
        ThreadQueue = nullptr;
        ThreadDispatcher = nullptr;

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
        std::cerr << "Dispatcher \"" << GetName() << "\" terminating (compeleted " << m_TasksCompleted - m_Keepalives << " tasks)" << std::endl;
#endif
    }

    void
    DispatcherBase::Run(void)
    /*++
      Start dispatcher on a new thread
    --*/
    {
        m_Thread = std::thread(
            std::bind(
                &DispatcherBase::DispatchLoop,
                this
            )
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
    DispatcherBase::StopTask(
        void
    )
    {
        m_Stop = true;
    }

    void
    DispatcherBase::Stop(void)
    {
        this->PostTask(
            dispatch::bind(
                &DispatcherBase::StopTask,
                this
            ),
            TaskPriority::PRIORITY_HIGH
        );
    }

    void
    DispatcherBase::PostTaskInternal(
        Job TaskJob
    )
    {
        if (std::this_thread::get_id() == m_ThreadId)
        {
            if (TaskJob.IsDelayed())
            {
                auto time = TaskJob.GetDispatchTime();
                m_DelayedTasks++;
                if (time < m_NextDelayedTask)
                {
                    m_NextDelayedTask = time;
                }
            }
            m_Queue.push_back(std::move(TaskJob));
            m_ReceivedTask = true;
        }
        else
        {
            {
                std::lock_guard<std::mutex> guard(m_CrossThreadMutex);
                m_CrossThread.push_back(std::move(TaskJob));
            }
            m_TaskAvailable.notify_one();
        }
    }

    void
    DispatcherBase::PostDelayedTask(
        const Callable& Task,
        const std::chrono::microseconds Delay
    )
    {
        auto triggerTime = std::chrono::system_clock::now() + Delay;
        auto job = Job(std::move(Task), this, triggerTime);
        assert(job.IsDelayed());
        PostTaskInternal(std::move(job));
    }

    void
    Dispatcher::PostTask(
        const Callable& Task,
        const TaskPriority Priority
    )
    {
        auto job = Job(std::move(Task), Priority, this);
        this->DispatcherBase::PostTaskInternal(std::move(job));
    }

    void
    Dispatcher::PostTaskAndReply(
        const Callable& Task,
        const Callable& Reply,
        const TaskPriority Priority
    )
    {
#ifdef DEBUG
        static bool warned = false;
        if(std::this_thread::get_id() != m_ThreadId && !warned)
        {
            std::cerr << "[!] WARNING: PostTaskAndReply called on same thread." << std::endl;
            warned = true;
        }
#endif
        
        //
        // In order to reply, we must already be
        // on a functioning dispatcher
        //
        assert(ThreadQueue != nullptr);
        
        auto reply = Job(std::move(Reply), Priority, ThreadQueue);
        auto job = Job(std::move(Task), Priority, this, reply);
        this->DispatcherBase::PostTaskInternal(std::move(job));
    }

}