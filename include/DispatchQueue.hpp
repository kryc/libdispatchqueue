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

    using DispatcherPtr = std::shared_ptr<DispatcherBase>;

    DispatcherPtr CreateDispatcher(void);
    DispatcherPtr CreateDispatcher(const Callable& EntryPoint);
    DispatcherPtr CreateDispatcher(const std::string& Name);
    DispatcherPtr CreateDispatcher(const std::string& Name, const Callable& EntryPoint);
    DispatcherPtr GetDispatcher(std::string Name);
    template <typename DispatcherPtrT>
    void PostTaskToDispatcher(DispatcherPtrT Dispatcher, const Callable& Job);
    void PostTaskToDispatcher(const std::string& Name, const Callable& Job);
    void PostTask(const Callable& Job);

    void GlobalDispatcherWait(void);
    
    //
    // Template specialisations
    //

    template <typename DispatcherPtrT>
    void PostTaskToDispatcher(DispatcherPtrT Dispatcher, const Callable& Job)
    {
        Dispatcher->PostTask(Job);
    }
    
}