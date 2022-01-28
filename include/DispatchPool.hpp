#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include "DispatcherBase.hpp"

namespace dispatch
{

    class DispatchPool : public DispatcherBase
    {
    public:
        DispatchPool(void) = delete;
        DispatchPool(const std::string& Name, const size_t Size = 0);
        void PostTask(const Callable& Task, const TaskPriority Priority = TaskPriority::PRIORITY_NORMAL) override;
        void PostTaskAndReply(const Callable& Task, const Callable& Reply, const TaskPriority Priority = TaskPriority::PRIORITY_NORMAL) override;
        ~DispatchPool(void) = default;
        void Run(void) override {return;};
        void Stop(void) override;
        bool Wait(void) override;
    protected:
        void OnDispatcherTerminated(DispatcherBase* Dispatacher);
    private:
        size_t Next(void);
        std::vector<DispatcherUPtr> m_Dispatchers;
        size_t m_NextDispatcher;
        std::atomic<size_t> m_Active;
    };

    using DispatchPoolPtr = std::shared_ptr<DispatchPool>;
    using DispatcherPoolPtr = DispatchPoolPtr;
}