#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <mutex>
#include <algorithm>
#include <vector>
#include <assert.h>
#include <condition_variable>

#include "DispatchQueue.hpp"

namespace dispatch
{
    std::mutex g_DispatcherMutex;
    std::map<std::string, DispatcherPtr> g_Dispatchers;
    std::vector<DispatcherPtr> g_AnonymousDispatchers;

    std::mutex g_GlobalWaitMutex;
    std::condition_variable g_GlobalWaitCondition;

    thread_local DispatcherBase* ThreadQueue = nullptr;

    void
    OnDispatcherDestroyed(DispatcherBase* Dispatcher)
    {
        std::lock_guard<std::mutex> mutex(g_DispatcherMutex);
        for (auto dispatcher = g_Dispatchers.begin(); dispatcher != g_Dispatchers.end(); dispatcher++)
        {
            if (std::get<1>(*dispatcher).get() == Dispatcher)
            {
                assert(Dispatcher->Completed());
                std::unique_lock<std::mutex> lk(g_GlobalWaitMutex);
                g_Dispatchers.erase(dispatcher);
                lk.unlock();
                g_GlobalWaitCondition.notify_one();
                break;
            }
        }
        for (auto dispatcher = g_AnonymousDispatchers.begin(); dispatcher != g_AnonymousDispatchers.end(); dispatcher++)
        {
            if (dispatcher->get() == Dispatcher)
            {
                assert(Dispatcher->Completed());
                std::unique_lock<std::mutex> lk(g_GlobalWaitMutex);
                g_AnonymousDispatchers.erase(dispatcher);
                lk.unlock();
                g_GlobalWaitCondition.notify_one();
                break;
            }
        }
    }

    DispatcherBase*
    CurrentQueue(void)
    {
        return ThreadQueue;
    }

    DispatcherPtr
    CreateDispatcher(
        const std::string& Name,
        const Callable& Entrypoint
    )
    {
        std::lock_guard<std::mutex> mutex(g_DispatcherMutex);
        auto dispatcher = std::make_shared<DispatcherBase>(Name, std::ref(Entrypoint));
        g_Dispatchers[Name] = dispatcher;
        dispatcher->SetDestructionHandler(std::bind(&OnDispatcherDestroyed, std::placeholders::_1));
        dispatcher->Run();
        return dispatcher;
    }

    DispatcherPtr
    CreateDispatcher(
        const std::string& Name
    )
    {
        std::lock_guard<std::mutex> mutex(g_DispatcherMutex);
        auto dispatcher = std::make_shared<DispatcherBase>(Name);
        g_Dispatchers[Name] = dispatcher;
        dispatcher->SetDestructionHandler(std::bind(&OnDispatcherDestroyed, std::placeholders::_1));
        dispatcher->Run();
        return dispatcher;
    }

    DispatcherPtr
    CreateDispatcher(
        void
    )
    /*++
      Create an anonymous dispatcher. This will not be added to the
      global named map but will be tracked in a separate vector
    --*/
    {
        std::lock_guard<std::mutex> mutex(g_DispatcherMutex);
        auto dispatcher = std::make_shared<DispatcherBase>();
        g_AnonymousDispatchers.push_back(dispatcher);
        dispatcher->SetDestructionHandler(std::bind(&OnDispatcherDestroyed, std::placeholders::_1));
        dispatcher->Run();
        return dispatcher;
    }

    DispatcherPtr
    CreateDispatcher(
        const Callable& Entrypoint
    )
    {
        std::lock_guard<std::mutex> mutex(g_DispatcherMutex);
        auto dispatcher = std::make_shared<DispatcherBase>(std::ref(Entrypoint));
        g_AnonymousDispatchers.push_back(dispatcher);
        dispatcher->SetDestructionHandler(std::bind(&OnDispatcherDestroyed, std::placeholders::_1));
        dispatcher->Run();
        return dispatcher;
    }

    DispatcherPtr
    GetDispatcher(std::string Name)
    {
        std::lock_guard<std::mutex> mutex(g_DispatcherMutex);
        if (g_Dispatchers.find(Name) == g_Dispatchers.end())
        {
            return nullptr;
        }
        return g_Dispatchers.at(Name);
    }

    void
    PostTaskToDispatcher(
        const std::string& Name,
        const Callable& Job
    )
    {
        auto dispatcher = GetDispatcher(Name);
        if (dispatcher)
        {
            PostTaskToDispatcher(dispatcher, std::move(Job));
        }
        else
        {
            std::cerr << "Dispatcher " << Name << " not found" << std::endl;
        }
    }

    void
    PostTaskToDispatcher(
        DispatcherPtr Dispatcher,
        const Callable& Job
    )
    {
        Dispatcher->PostTask(std::move(Job));
    }

    void
    PostTaskToDispatcher(
        DispatcherBase* Dispatcher,
        const Callable& Job
    )
    {
        Dispatcher->PostTask(std::move(Job));
    }

    void
    PostTaskAndReply(
        DispatcherPtr Dispatcher,
        const Callable& Job,
        const Callable& Reply
    )
    {
        Dispatcher->PostTaskAndReply(std::move(Job), std::move(Reply));
    }

    void
    PostTaskAndReply(
        const std::string& Name,
        const Callable& Job,
        const Callable& Reply
    )
    {
        auto dispatcher = GetDispatcher(Name);
        PostTaskAndReply(dispatcher, Job, Reply);
    }

    void
    PostTask(
        const Callable& Job
    )
    /*++
      Post a task to the current task queue
      --*/
    {
        auto dispatcher = ThreadQueue;
        PostTaskToDispatcher(dispatcher, Job);
    }

    bool
    OnDispatcher(
        const std::string& Name
    )
    /*++
      Checks if executing on the specified named
      dispatcher. For anonymous dispatcher use ""
      --*/
    {
        return ThreadQueue->GetName() == Name;
    }

    void
    KeepAlive(
        const bool KeepAlive
    )
    /*++
      No longer keep this dispatcher alive
      This dispatcher will be destroyed when no tasks
      remain on the queue
      --*/
    {
        ThreadQueue->KeepAlive(KeepAlive);
    }

    void
    End(
        void
    )
    /*++
      End the current dispatcher
    --*/
    {
        auto dispatcher = ThreadQueue;
        dispatcher->Stop();
    }

    void
    GlobalDispatcherWait(
        void
    )
    {
        std::unique_lock<std::mutex> lk(g_GlobalWaitMutex);
        g_GlobalWaitCondition.wait(lk, []{ return g_Dispatchers.size() == 0 && g_AnonymousDispatchers.size() == 0; });
    }

}