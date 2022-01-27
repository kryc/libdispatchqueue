#include <functional>
#include <iostream>
#include <memory>
#include <sstream>

#include "DispatchPool.hpp"

namespace dispatch
{

    DispatchPool::DispatchPool(const size_t Size, const std::string& Name)
    :DispatcherBase::DispatcherBase(Name)
    {
        m_NextDispatcher = 0;
        m_Active = Size;
        for (size_t i = 0; i < Size; i++)
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
    {
        m_Dispatchers[Next()]->PostTask(Task, Priority);
    }

    void
    DispatchPool::PostTaskAndReply(
        const Callable& Task,
        const Callable& Reply,
        const TaskPriority Priority
    )
    {
        m_Dispatchers[Next()]->PostTaskAndReply(
            Task,
            Reply,
            Priority
        );
    }

    void
    DispatchPool::Run(
        void
    )
    {
        return;
    }

    void
    DispatchPool::Stop(
        void
    )
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
    {
        m_Active--;
        if (m_Active == 0)
        {
            m_DestructionHandler(this);
        }
    }
}