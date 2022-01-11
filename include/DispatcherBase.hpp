#pragma once

#include <deque>
#include "Callable.hpp"
#include "Job.hpp"

namespace dispatch{

    class DispatcherBase
    {
    public:
        DispatcherBase(void) = default;
        DispatcherBase(Callable& Entrypoint);
        ~DispatcherBase(void);
        void Run(void);
        void PostTask(const Callable& Entrypoint);
        void Wait(void);
        void Stop(void);
        bool Stopped(void) { return m_Completed; };
        bool Completed(void) { return m_Completed; };
    private:
        void DispatchLoop(void);
        std::deque<Job> m_Queue;
        std::thread m_Thread;
        bool m_Stop = false;
        bool m_Completed = false;
    };

}