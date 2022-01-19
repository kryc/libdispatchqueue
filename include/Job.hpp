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
        Job(Callable Entrypoint, const TaskPriority Priority, const void* Dispatcher, Job& Reply) :
            m_Entrypoint(Entrypoint),
            m_Priority(Priority),
            m_Dispatcher(Dispatcher) { m_Reply = std::make_unique<Job>(std::move(Reply)); };
        Job(Job&& Other) :
            m_Entrypoint(Other.m_Entrypoint),
            m_Priority(Other.m_Priority),
            m_Dispatcher(Other.m_Dispatcher) { m_Reply = std::move(Other.m_Reply); };
        const TaskPriority GetPriority(void) { return m_Priority; };
        const void* GetDispatcher(void) { return m_Dispatcher; };
        void operator()() { m_Entrypoint(); };
        bool operator< (Job Comparitor) { return Comparitor.GetPriority() < m_Priority; };
        bool HasReply(void) { return m_Reply != nullptr; };
        std::unique_ptr<Job> GetReply(void) { return std::move(m_Reply); };
    protected:
        const Callable m_Entrypoint;
        const TaskPriority m_Priority = PRIORITY_NORMAL;
        const void* m_Dispatcher = nullptr;
        std::unique_ptr<Job> m_Reply;
    };

}