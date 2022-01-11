#pragma once

#include <functional>
#include <memory>
#include <thread>
#include <future>

#include "Job.hpp"
#include "DispatcherBase.hpp"

namespace dispatch
{

    extern thread_local DispatcherBase* ThreadQueue;

    DispatcherBase* CurrentQueue(void);

    std::shared_ptr<DispatcherBase> CreateDispatcher(std::string Name);
    std::shared_ptr<DispatcherBase> CreateDispatcher(std::string Name, Callable& EntryPoint);
    std::shared_ptr<DispatcherBase> GetDispatcher(std::string Name);
    void PostTaskToDispatcher(std::string Name, const Callable& Job);

    void GlobalDispatcherWait(void);
}