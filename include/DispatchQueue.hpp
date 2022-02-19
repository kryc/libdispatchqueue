#pragma once

#include <functional>
#include <memory>
#include <thread>
#include <future>
#include <string>

#include "Job.hpp"
#include "DispatcherBase.hpp"
#include "DispatchPool.hpp"

namespace dispatch
{
    extern thread_local DispatcherBase* ThreadQueue;
    extern thread_local DispatcherBase* ThreadDispatcher;

    DispatcherBase* CurrentQueue(void);
    DispatcherBase* CurrentDispatcher(void);

    DispatcherBasePtr CreateDispatcher(void);
    DispatcherBasePtr CreateDispatcher(const Callable& EntryPoint);
    DispatcherBasePtr CreateDispatcher(const std::string& Name);
    DispatcherBasePtr CreateDispatcher(const std::string& Name, const Callable& EntryPoint);
    DispatcherBasePtr CreateAndEnterDispatcher(const std::string& Name, const Callable& EntryPoint);
    DispatcherPoolPtr CreateDispatchPool(const std::string& Name, const size_t Size = 0);
    DispatcherBasePtr GetDispatcher(std::string Name);
    void RemoveDispatcher(DispatcherBase* Dispatcher);
    void PostTaskToDispatcher(DispatcherBasePtr Dispatcher, const Callable& Job);
    void PostTaskToDispatcher(const std::string& Name, const Callable& Job);
    void PostDelayedTaskToDispatcher(DispatcherBasePtr Dispatcher, const Callable& Job, const std::chrono::microseconds Delay);
    void PostTaskAndReply(DispatcherBasePtr Dispatcher, const Callable& Job, const Callable& Reply);
    void PostTaskAndReply(const std::string& Name, const Callable& Job, const Callable& Reply);
    void PostTask(DispatcherBase* Dispatcher, const Callable& Job);
    void PostDelayedTask(DispatcherBase* Dispatcher, const Callable& Job, const std::chrono::microseconds Delay);
    void PostTask(const Callable& Job);
    void PostTaskStrict(const Callable& Job);
    void PostTaskFast(const Callable& Job);
    void PostDelayedTask(const Callable& Job, const std::chrono::microseconds Delay);

    bool OnDispatcher(const std::string& Name);
    void KeepAlive(const bool KeepAlive);

    void End(void);

    void GlobalDispatcherWait(void);
}