#pragma once

#include <atomic>
#include <deque>
#include <string>
#include "Callable.hpp"
#include "Job.hpp"

namespace dispatch{

    class DispatcherBase
    {
    public:
        DispatcherBase(void) = default;
        DispatcherBase(const std::string& Name) : m_Name(Name){};
        DispatcherBase(const Callable& Entrypoint);
        DispatcherBase(const std::string& Name, const Callable& Entrypoint);
        ~DispatcherBase(void);
        void Run(void);
        void PostTask(const Callable& Entrypoint);
        bool Wait(void);
        void Stop(void);
        void KeepAlive(const bool KeepAlive) { m_KeepAlive = KeepAlive; };
        bool Stopped(void) { return m_Completed; };
        bool Completed(void) { return m_Completed; };
        std::string GetName(void) { return IsAnonymous() ? "ANON" : m_Name; };
        bool IsAnonymous(void) { return m_Name.empty(); };
    private:
        void KeepAliveInternal(void);
        void DispatchLoop(void);
        std::deque<Job> m_Queue;
        std::thread m_Thread;
        std::string m_Name;
        bool m_KeepAlive = false;
        bool m_Stop = false;
        bool m_Completed = false;
        std::atomic<bool> m_Waiting;
    };

}