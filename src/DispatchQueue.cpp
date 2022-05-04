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
#include "DispatchPool.hpp"

namespace dispatch
{
    std::mutex g_DispatcherMutex;
    std::map<std::string, DispatcherBasePtr> g_Dispatchers;
    std::vector<DispatcherBase*> g_CompletedDispatchers;

    std::atomic<size_t> g_ActiveDispatcherCount = 0;
    std::atomic<size_t> g_TotalDispatchers = 0;

    std::condition_variable g_GlobalWaitCondition;

    thread_local DispatcherBase* ThreadQueue = nullptr;
    thread_local DispatcherBase* ThreadDispatcher = nullptr;

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

    DispatcherBase*
    CurrentDispatcher(
        void
    )
    {
        return ThreadDispatcher == nullptr ? ThreadQueue : ThreadDispatcher;
    }

    static void
    TrackDispatcher(
        const std::string& Name,
        DispatcherBasePtr Dispatcher
    )
    {
        std::lock_guard<std::mutex> mutex(g_DispatcherMutex);
        g_Dispatchers[Name] = Dispatcher;
        g_ActiveDispatcherCount++;
        g_TotalDispatchers++;
    }

    DispatcherBasePtr
    CreateAndEnterDispatcher(
        const std::string& Name,
        const Callable& Entrypoint)
    {
        
        auto dispatcher = std::make_shared<Dispatcher>(Name);
        dispatcher->PostTask(Entrypoint);
        dispatcher->SetDestructionHandler(std::bind(&OnDispatcherDestroyed, std::placeholders::_1));
        TrackDispatcher(Name, dispatcher);
        dispatcher->Enter();
        return dispatcher;
    }

    DispatcherBasePtr
    CreateDispatcher(
        const std::string& Name,
        const Callable& Entrypoint
    )
    {
        auto dispatcher = std::make_shared<Dispatcher>(Name);
        dispatcher->PostTask(Entrypoint);
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

    DispatcherBasePtr
    CreateDispatcher(
        const std::string& Name
    )
    {
        return CreateDispatcher(Name, dispatch::bind(&NullEntrypoint));
    }

    DispatcherBasePtr
    CreateDispatcher(
        const Callable& Entrypoint
    )
    {
        std::stringstream name;
        name << "anonymous" << g_TotalDispatchers++;
        return CreateDispatcher(name.str(), Entrypoint);
    }

    DispatcherBasePtr
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

    DispatcherPoolPtr
    CreateDispatchPool(
        const std::string& Name,
        const size_t Size
    )
    {
        auto dispatcher = std::make_shared<DispatchPool>(Name, Size);
        dispatcher->SetDestructionHandler(std::bind(&OnDispatcherDestroyed, std::placeholders::_1));
        TrackDispatcher(Name, dispatcher);
        dispatcher->Run();
        return dispatcher;
    }

    DispatcherBasePtr
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
        DispatcherBasePtr Dispatcher,
        const Callable& Job
    )
    {
        Dispatcher->PostTask(std::move(Job));
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
        DispatcherBase* Dispatcher,
        const Callable& Job
    )
    {
        Dispatcher->PostTask(std::move(Job));
    }

    void
    PostDelayedTaskToDispatcher(
        DispatcherBase* Dispatcher,
        const Callable& Job,
        const std::chrono::microseconds Delay
    )
    {
        Dispatcher->PostDelayedTask(std::move(Job), Delay);
    }

    void
    PostDelayedTaskToDispatcher(
        DispatcherBasePtr Dispatcher,
        const Callable& Job,
        const std::chrono::microseconds Delay
    )
    {
        Dispatcher->PostDelayedTask(std::move(Job), Delay);
    }

    void
    PostDelayedTaskToDispatcher(
        const std::string& Name,
        const Callable& Job,
        const std::chrono::microseconds Delay
    )
    {
        auto dispatcher = GetDispatcher(Name);
        if (dispatcher)
        {
            PostDelayedTaskToDispatcher(dispatcher, std::move(Job), Delay);
        }
        else
        {
            std::cerr << "Dispatcher " << Name << " not found" << std::endl;
        }
    }

    void
    PostTaskAndReply(
        DispatcherBasePtr Dispatcher,
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
    PostDelayedTask(
        const Callable& Job,
        const std::chrono::microseconds Delay
    )
    {
        auto dispatcher = CurrentDispatcher();
        PostDelayedTaskToDispatcher(dispatcher, Job, Delay);
    }

    void
    PostDelayedTaskStrict(
        const Callable& Job,
        const std::chrono::microseconds Delay
    )
    {
        auto dispatcher = CurrentQueue();
        PostDelayedTaskToDispatcher(dispatcher, Job, Delay);
    }

    void
    PostTask(
        const Callable& Job
    )
    /*++
      Post a task to the current dispatcher
      --*/
    {
        auto dispatcher = CurrentDispatcher();
        PostTaskToDispatcher(dispatcher, Job);
    }

    void
    PostTaskStrict(
        const Callable& Job
    )
    /*++
      Post a task to the current queue strictly
      disregarding the dispatcher
      This will bypass any pool logic
    --*/
    {
        auto dispatcher = CurrentQueue();
        PostTaskToDispatcher(dispatcher, Job);
    }

    void
    PostTaskFast(
        const Callable& Job
    )
    /*++
      The fastest way to post a task is to the
      current queue as it bypasses any locking
    --*/
    {
        PostTaskStrict(Job);
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
        return CurrentDispatcher()->GetName() == Name;
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
        auto dispatcher = CurrentQueue();
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