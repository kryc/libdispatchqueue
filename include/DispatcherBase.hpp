#pragma once

#include <atomic>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <string>
#include <thread>
#include "Callable.hpp"
#include "Job.hpp"

namespace dispatch{

    class DispatcherBase;
    using DestructionHandler = std::function<void(DispatcherBase*)>;

    class DispatcherBase
    {
    public:
        DispatcherBase(void) = delete;
        DispatcherBase(const std::string& Name) : m_Name(Name) {};
        virtual ~DispatcherBase(void);
        virtual void Run(void);
        void Enter(void);
        virtual void PostTask(const Callable& Task, const TaskPriority Priority = TaskPriority::PRIORITY_NORMAL) = 0;
        virtual void PostTaskAndReply(const Callable& Task, const Callable& Reply, const TaskPriority Priority = TaskPriority::PRIORITY_NORMAL) = 0;
        virtual bool Wait(void);
        virtual void Stop(void);
        void KeepAlive(const bool KeepAlive) { m_KeepAlive = KeepAlive; };
        bool Stopped(void) { return m_Completed; };
        bool Completed(void) { return m_Completed; };
        std::string GetName(void) { return m_Name; };
        void SetDestructionHandler(DestructionHandler Handler) { m_DestructionHandler = Handler; };
    protected:
        void PostTask(Job TaskJob);
        void KeepAliveInternal(void);
        void DispatchLoop(void);
        void StopTask(void);
        std::deque<Job> m_Queue;
        std::mutex m_CrossThreadMutex;
        std::deque<Job> m_CrossThread;
        std::condition_variable m_TaskAvailable;
        std::thread m_Thread;
        std::thread::id m_ThreadId;
        std::string m_Name;
        bool m_ReceivedTask = false;
        bool m_KeepAlive = true;
        bool m_Stop = false;
        bool m_Completed = false;
        std::atomic<bool> m_Waiting;

        DestructionHandler m_DestructionHandler;
    };

    class Dispatcher : public DispatcherBase
    {
    public:
        Dispatcher(const std::string& Name):DispatcherBase::DispatcherBase(Name){};
        // ~Dispatcher(void) { DispatcherBase::~DispatcherBase(); } override;
        void PostTask(const Callable& Task, const TaskPriority Priority = TaskPriority::PRIORITY_NORMAL) override;
        void PostTaskAndReply(const Callable& Task, const Callable& Reply, const TaskPriority Priority = TaskPriority::PRIORITY_NORMAL) override;
        ~Dispatcher(void) = default;
        using DispatcherBase::Run;
    protected:
        using DispatcherBase::PostTask;
    };

    using DispatcherBasePtr = std::shared_ptr<DispatcherBase>;
    using DispatcherBaseUPtr = std::unique_ptr<DispatcherBase>;
    using DispatcherPtr = std::shared_ptr<Dispatcher>;
    using DispatcherUPtr = std::unique_ptr<Dispatcher>;

}