#pragma once

#include <functional>
#include <memory>
#include <thread>
#include <future>
#include <string>

#include "Job.hpp"
#include "DispatcherBase.hpp"

namespace dispatch
{
    extern thread_local DispatcherBase* ThreadQueue;

    DispatcherBase* CurrentQueue(void);

    using DispatcherPtr = std::shared_ptr<DispatcherBase>;

    DispatcherPtr CreateDispatcher(void);
    DispatcherPtr CreateDispatcher(const Callable& EntryPoint);
    DispatcherPtr CreateDispatcher(const std::string& Name);
    DispatcherPtr CreateDispatcher(const std::string& Name, const Callable& EntryPoint);
    DispatcherPtr GetDispatcher(std::string Name);
    void PostTaskToDispatcher(DispatcherPtr Dispatcher, const Callable& Job);
    void PostTaskToDispatcher(const std::string& Name, const Callable& Job);
    void PostTaskAndReply(DispatcherPtr Dispatcher, const Callable& Job, const Callable& Reply);
    void PostTaskAndReply(const std::string& Name, const Callable& Job, const Callable& Reply);
    void PostTask(const Callable& Job);

    bool OnDispatcher(const std::string& Name);
    void KeepAlive(const bool KeepAlive);

    void End(void);

    void GlobalDispatcherWait(void);
}