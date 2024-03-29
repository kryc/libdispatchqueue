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
        auto index = (m_Dispatched++) % m_Dispatchers.size();
        return m_Dispatchers[index].get();
    }
    
    void
    DispatchPool::PostTask(
        const Callable& Task,
        const TaskPriority Priority
    )
    /*++
      Post a task to the next dispatcher.
      If the current queue is empty then always post the
      task here as it is significantly faster
    --*/
    {
        // auto currentDispatcher = CurrentQueue();
        // if (currentDispatcher->Empty())
        // {
        //     currentDispatcher->PostTask(Task, Priority);
        // }
        // else
        // {
        //     Next()->PostTask(Task, Priority);
        // }
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
        for (auto& dispatcher : m_Dispatchers)
        {
            dispatcher->Stop();
        }
    }

    void
    DispatchPool::Start(
        void
    )
    /*++
      Request each of the dispatchers to start
    --*/
    {
        for (auto& dispatcher : m_Dispatchers)
        {
            dispatcher->Start();
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
            NotifyDestruction();
        }
    }
}