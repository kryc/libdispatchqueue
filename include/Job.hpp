#pragma once

#include "Callable.hpp"

namespace dispatch
{

    typedef enum : char
    {
        PRIORITY_LOW = 3,
        PRIORITY_NORMAL = 2,
        PRIORITY_HIGH = 1
    } TaskPriority;

    class Job
    {
    public:
        Job(Callable Entrypoint, const TaskPriority Priority, const void* Dispatcher) :
            m_Entrypoint(Entrypoint),
            m_Priority(Priority),
            m_Dispatcher(Dispatcher) {};
        const TaskPriority GetPriority(void) { return m_Priority; };
        const void* GetDispatcher(void) { return m_Dispatcher; };
        void operator()() { m_Entrypoint(); };
        bool operator< (Job Comparitor) { return Comparitor.GetPriority() < m_Priority; };
    private:
        const Callable m_Entrypoint;
        const TaskPriority m_Priority = PRIORITY_NORMAL;
        const void*  m_Dispatcher = nullptr;
    };

}