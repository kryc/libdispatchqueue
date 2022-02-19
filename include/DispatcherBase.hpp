#pragma once

#include <atomic>
#include <chrono>
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
    using CompletionHandler = std::function<void(DispatcherBase*)>;

    extern const dispatch::timepoint MAXTIME;

    class DispatcherBase
    {
    public:
        DispatcherBase(void) = delete;
        DispatcherBase(const std::string& Name) : m_Name(Name) {};
        virtual ~DispatcherBase(void);
        virtual void Run(void);
        void Enter(void);
        virtual void PostTask(const Callable& Task, const TaskPriority Priority = TaskPriority::PRIORITY_NORMAL) = 0;
        void PostDelayedTask(const Callable& Task, const std::chrono::microseconds Delay);
        virtual void PostTaskAndReply(const Callable& Task, const Callable& Reply, const TaskPriority Priority = TaskPriority::PRIORITY_NORMAL) = 0;
        virtual bool Wait(void);
        virtual void Stop(void);
        void KeepAlive(const bool KeepAlive) { m_KeepAlive = KeepAlive; };
        bool Stopped(void) { return m_Completed; };
        bool Completed(void) { return m_Completed; };
        std::string GetName(void) { return m_Name; };
        void SetDestructionHandler(DestructionHandler Handler) { m_DestructionHandler = Handler; };
        void SetCompletionHandler(CompletionHandler Handler) { m_CompletionHandler = Handler; };
        void SetThreadDispatcher(DispatcherBase* Dispatcher);
    protected:
        void PostTaskInternal(Job TaskJob);
        void NotifyCompletion(void) { if (m_CompletionHandler) m_CompletionHandler(this); };
        void NotifyDestruction(void) { if (m_DestructionHandler) m_DestructionHandler(this); };
        std::deque<Job> m_Queue;
        std::deque<Job> m_DelayedQueue;
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
        size_t m_TasksCompleted = 0;
        size_t m_Keepalives = 0;
        std::atomic<bool> m_Waiting;
        dispatch::timepoint m_NextDelayedTask = dispatch::MAXTIME;
    private:
        void StopTask(void);
        void KeepAliveInternal(void);
        void DispatchLoop(void);
        void DispatchJob(Job ToRun);

        CompletionHandler m_CompletionHandler;
        DestructionHandler m_DestructionHandler;
        DispatcherBase* m_ThreadDispatcher = nullptr;
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