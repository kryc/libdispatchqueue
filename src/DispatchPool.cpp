#include <functional>
#include <iostream>
#include <memory>
#include <sstream>

#include "DispatchPool.hpp"

namespace dispatch
{

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
        m_NextDispatcher = 0;
        m_Active = count;

        //
        // Initialize the dispatchers
        //
        for (size_t i = 0; i < count; i++)
        {
            std::stringstream dispatcher_name;
            dispatcher_name << Name << "[" << i << "]";
            auto dispatcher = std::make_unique<Dispatcher>(dispatcher_name.str());
            dispatcher->SetDestructionHandler(
                std::bind(&DispatchPool::OnDispatcherTerminated, this, std::placeholders::_1)
            );
            dispatcher->Run();
            m_Dispatchers.push_back(std::move(dispatcher));
        }
    }

    size_t
    DispatchPool::Next(
        void
    )
    /*++
      Returns the next dispatcher to push tasks to.
      Base implementation is a simple round-robin
    --*/
    {
        if (m_NextDispatcher >= m_Dispatchers.size())
        {
            m_NextDispatcher = 0;
        }
        return m_NextDispatcher++;
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
        m_Dispatchers.at(Next())->PostTask(Task, Priority);
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
        m_Dispatchers.at(Next())->PostTaskAndReply(
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
            m_DestructionHandler(this);
        }
    }
}