#include <functional>
#include <iostream>
#include <memory>
#include <sstream>

#include "DispatchPool.hpp"

namespace dispatch
{

    DispatcherBase* CurrentQueue(void);

    DispatchPool::DispatchPool(const std::string& Name, const size_t Size)
    :DispatcherBase::DispatcherBase(Name)
    {
        //
        // Decide if we use the number of available cores
        //
        size_t count = Size;
        if (count == 0)
        {
            count = std::thread::hardware_concurrency();
        }

        //
        // Configure counters
        //
        m_Active = count;

        //
        // Initialize the dispatchers
        //
        for (size_t i = 0; i < count; i++)
        {
            std::stringstream dispatcher_name;
            dispatcher_name << Name << "[" << i << "]";
            auto dispatcher = std::make_unique<Dispatcher>(dispatcher_name.str());
            dispatcher->SetThreadDispatcher(this);
            dispatcher->SetDestructionHandler(
                std::bind(&DispatchPool::OnDispatcherTerminated, this, std::placeholders::_1)
            );
            dispatcher->Run();
            m_FreeList.push_back(dispatcher.get());
            m_Dispatchers.push_back(std::move(dispatcher));
        }
    }

    DispatcherBase*
    DispatchPool::Next(
        void
    )
    /*++
      Returns the next dispatcher to push tasks to.
      Base implementation is a simple round-robin
    --*/
    {
        std::lock_guard<std::mutex> mutex(m_FreeListMutex);
        
        //
        // Try and use the free list where possible
        //
        if (!m_FreeList.empty())
        {
            auto dispatcher = m_FreeList.back();
            m_FreeList.pop_back();
            return dispatcher;
        }

        //
        // Try the current dispatcher if we are on one
        //
        auto current = dispatch::CurrentQueue();
        if (current != nullptr)
        {
            return current;
        }

        //
        // Fallback to the first dispatcher
        //
        auto dispatcher = std::move(m_Dispatchers.front().get());
        return dispatcher;
    }
    
    void
    DispatchPool::PostTask(
        const Callable& Task,
        const TaskPriority Priority
    )
    /*++
      Post a task to the next dispatcher
    --*/
    {
        Next()->PostTask(Task, Priority);
    }

    void
    DispatchPool::PostTaskAndReply(
        const Callable& Task,
        const Callable& Reply,
        const TaskPriority Priority
    )
    /*++
      Post a task with reply to the next dispatcher
    --*/
    {
        Next()->PostTaskAndReply(
            Task,
            Reply,
            Priority
        );
    }

    void
    DispatchPool::Stop(
        void
    )
    /*++
      Request each of the dispatchers to stop
    --*/
    {
        std::lock_guard<std::mutex> mutex(m_FreeListMutex);
        for (auto& dispatcher : m_Dispatchers)
        {
            dispatcher->Stop();
        }
    }

    bool
    DispatchPool::Wait(
        void
    )
    /*++
      Wait on each of the dispatchers. Only return
      once all have completed
    --*/
    {
        for (auto& dispatcher : m_Dispatchers)
        {
            dispatcher->Wait();
        }
        return true;
    }

    void
    DispatchPool::OnDispatcherCompleted(
        DispatcherBase* Dispatcher
    )
    {
        std::lock_guard<std::mutex> mutex(m_FreeListMutex);
        m_FreeList.push_back(Dispatcher);
    }

    void
    DispatchPool::OnDispatcherTerminated(
        DispatcherBase* Dispatacher
    )
    /*++
      Callback for the termination of a dispatcher
      Update active count and callback if zero
    --*/
    {
        m_Active--;
        if (m_Active == 0)
        {
            NotifyDestruction();
        }
    }
}