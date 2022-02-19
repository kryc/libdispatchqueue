#pragma once

#include <string.h>

#include <chrono>

#include "Callable.hpp"

namespace dispatch
{

    enum class TaskPriority : char
    {
        PRIORITY_LOW = 3,
        PRIORITY_NORMAL = 2,
        PRIORITY_HIGH = 1
    };

    using timepoint = std::chrono::time_point<std::chrono::system_clock>;

    class Job
    {
    public:
        Job(Callable Entrypoint, const TaskPriority Priority, const void* Dispatcher) :
            m_Entrypoint(Entrypoint),
            m_Priority(Priority),
            m_Dispatcher(Dispatcher) {};
        Job(Callable Entrypoint, const void* Dispatcher, const timepoint Timepoint) :
            m_Entrypoint(Entrypoint),
            m_Priority(TaskPriority::PRIORITY_NORMAL),
            m_Dispatcher(Dispatcher),
            m_Delayed(true),
            m_DispatchTime(Timepoint) {};
        Job(Callable Entrypoint, const TaskPriority Priority, const void* Dispatcher, Job& Reply) :
            m_Entrypoint(Entrypoint),
            m_Priority(Priority),
            m_Dispatcher(Dispatcher) { m_Reply = std::make_unique<Job>(std::move(Reply)); };
        Job(Job& Other) = delete;
        Job(Job&& Other) :
            m_Entrypoint(std::move(Other.m_Entrypoint)),
            m_Priority(Other.m_Priority),
            m_Dispatcher(Other.m_Dispatcher),
            m_Delayed(Other.m_Delayed),
            m_DispatchTime(Other.m_DispatchTime),
            m_Reply(std::move(Other.m_Reply)) { Other.m_Dispatcher = nullptr; };
        Job& operator=(Job&& Other) = delete;
        Job& operator=(const Job& Other) = delete;
        const TaskPriority GetPriority(void) const { return m_Priority; };
        const void* GetDispatcher(void) const { return m_Dispatcher; };
        const bool IsDelayed(void) const { return m_Delayed; };
        const timepoint GetDispatchTime(void) const { return m_DispatchTime; };
        const bool ShouldRunNow(void) const { return !m_Delayed || std::chrono::system_clock::now() >= m_DispatchTime; };
        void operator()() { m_Entrypoint(); };
        bool operator< (Job Comparitor) const { return Comparitor.GetPriority() < m_Priority; };
        bool HasReply(void) const { return m_Reply != nullptr; };
        std::unique_ptr<Job> GetReply(void) { return std::move(m_Reply); };
    protected:
        const Callable m_Entrypoint;
        const TaskPriority m_Priority;
        const void* m_Dispatcher = nullptr;
        const bool m_Delayed = false;
        const timepoint m_DispatchTime;
        std::unique_ptr<Job> m_Reply;
    };

}