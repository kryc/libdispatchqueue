#include <atomic>
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
#include <sstream>

#include "DispatchQueue.hpp"

namespace dispatch
{
    std::mutex g_DispatcherMutex;
    std::map<std::string, DispatcherPtr> g_Dispatchers;
    std::vector<DispatcherBase*> g_CompletedDispatchers;

    std::atomic<size_t> g_ActiveDispatcherCount = 0;
    std::atomic<size_t> g_TotalDispatchers = 0;

    std::condition_variable g_GlobalWaitCondition;

    thread_local DispatcherBase* ThreadQueue = nullptr;

    void
    RemoveDispatcher(DispatcherBase* Dispatcher)
    {
        std::lock_guard<std::mutex> mutex(g_DispatcherMutex);
        for (auto dispatcher = g_Dispatchers.begin(); dispatcher != g_Dispatchers.end(); dispatcher++)
        {
            if (std::get<1>(*dispatcher).get() == Dispatcher)
            {
                assert(Dispatcher->Completed());
                g_Dispatchers.erase(dispatcher);
                break;
            }
        }
    }

    void
    OnDispatcherDestroyed(DispatcherBase* Dispatcher)
    {
        {
            std::lock_guard<std::mutex> mutex(g_DispatcherMutex);
            g_CompletedDispatchers.push_back(Dispatcher);
            g_ActiveDispatcherCount--;
        }
        g_GlobalWaitCondition.notify_one();
    }

    DispatcherBase*
    CurrentQueue(void)
    {
        return ThreadQueue;
    }

    static void
    TrackDispatcher(
        const std::string& Name,
        DispatcherPtr Dispatcher
    )
    {
        std::lock_guard<std::mutex> mutex(g_DispatcherMutex);
        g_Dispatchers[Name] = Dispatcher;
        g_ActiveDispatcherCount++;
        g_TotalDispatchers++;
    }

    void
    EnterDispatcher(
        const std::string& Name,
        const Callable& Entrypoint)
    {
        
        auto dispatcher = std::make_shared<DispatcherBase>(Name, std::ref(Entrypoint));
        dispatcher->SetDestructionHandler(std::bind(&OnDispatcherDestroyed, std::placeholders::_1));
        TrackDispatcher(Name, dispatcher);
        dispatcher->Enter();
    }

    DispatcherPtr
    CreateDispatcher(
        const std::string& Name,
        const Callable& Entrypoint
    )
    {
        auto dispatcher = std::make_shared<DispatcherBase>(Name, std::ref(Entrypoint));
        dispatcher->SetDestructionHandler(std::bind(&OnDispatcherDestroyed, std::placeholders::_1));
        TrackDispatcher(Name, dispatcher);
        dispatcher->Run();
        return dispatcher;
    }

    void
    NullEntrypoint(void)
    {
        return;
    }

    DispatcherPtr
    CreateDispatcher(
        const std::string& Name
    )
    {
        return CreateDispatcher(Name, dispatch::bind(&NullEntrypoint));
    }

    DispatcherPtr
    CreateDispatcher(
        const Callable& Entrypoint
    )
    {
        std::stringstream name;
        name << "anonymous" << g_TotalDispatchers++;
        return CreateDispatcher(name.str(), Entrypoint);
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
        std::stringstream name;
        name << "anonymous" << g_TotalDispatchers++;
        return CreateDispatcher(name.str());
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
        std::unique_lock<std::mutex> lk(g_DispatcherMutex);
        g_GlobalWaitCondition.wait(lk, []{ return g_ActiveDispatcherCount == 0; });
    }

}