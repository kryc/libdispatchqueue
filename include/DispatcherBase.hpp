#pragma once

#include <atomic>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <string>
#include "Callable.hpp"
#include "Job.hpp"

namespace dispatch{

    class DispatcherBase;
    using DestructionHandler = std::function<void(DispatcherBase*)>;

    class DispatcherBase
    {
    public:
        DispatcherBase(void) = delete;
        DispatcherBase(const std::string& Name, const Callable& Entrypoint);
        ~DispatcherBase(void);
        void Run(void);
        void Enter(void);
        void PostTask(Job& TaskJob);
        void PostTask(const Callable& Task, const TaskPriority Priority = PRIORITY_NORMAL);
        void PostTaskAndReply(const Callable& Task, const Callable& Reply, const TaskPriority Priority = PRIORITY_NORMAL);
        bool Wait(void);
        void Stop(void);
        void KeepAlive(const bool KeepAlive) { m_KeepAlive = KeepAlive; };
        bool Stopped(void) { return m_Completed; };
        bool Completed(void) { return m_Completed; };
        std::string GetName(void) { return m_Name; };
        void SetDestructionHandler(DestructionHandler Handler) { m_DestructionHandler = Handler; };
    private:
        void KeepAliveInternal(void);
        void DispatchLoop(void);
        std::deque<Job> m_Queue;
        std::mutex m_CrossThreadMutex;
        std::deque<Job> m_CrossThread;
        std::thread m_Thread;
        std::thread::id m_ThreadId;
        std::string m_Name;
        bool m_ReceivedTask = false;
        bool m_KeepAlive = true;
        bool m_Stop = false;
        bool m_Completed = false;
        std::atomic<bool> m_Waiting;

        DestructionHandler m_DestructionHandler;

        std::mutex m_ConditionMutex;
        std::condition_variable m_TaskAvailable;
    };

}