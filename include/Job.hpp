#pragma once

#include "Callable.hpp"

namespace dispatch
{

    typedef enum
    {
        PRIORITY_NORMAL
    } TaskPriority;

    class Job
    {
    public:
        Job(Callable Entrypoint, const TaskPriority Priority, const void* Dispatcher) :
            m_Entrypoint(Entrypoint),
            m_Priority(Priority),
            m_Dispatcher(Dispatcher) {};
        TaskPriority GetPriority(void) { return m_Priority; };
        const void* GetDispatcher(void) { return m_Dispatcher; };
        void operator()() { m_Entrypoint(); };
    private:
        const Callable m_Entrypoint;
    const TaskPriority m_Priority = PRIORITY_NORMAL;
    const void*  m_Dispatcher = nullptr;
    };

}