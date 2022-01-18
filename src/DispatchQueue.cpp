#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <mutex>
#include <algorithm>
#include <vector>

#include "DispatchQueue.hpp"

namespace dispatch
{
    std::mutex g_DispatcherMutex;
    std::map<std::string, DispatcherPtr> g_Dispatchers;
    std::vector<DispatcherPtr> g_AnonymousDispatchers;

    thread_local DispatcherBase* ThreadQueue = nullptr;

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
        dispatcher->Run();
        return dispatcher;
    }

    DispatcherPtr
    GetDispatcher(std::string Name)
    {
        std::lock_guard<std::mutex> mutex(g_DispatcherMutex);
        return g_Dispatchers.at(Name);
    }

    void
    PostTaskToDispatcher(const std::string& Name, const Callable& Job)
    {
        auto dispatcher = GetDispatcher(Name);
        PostTaskToDispatcher(dispatcher, Job);
    }

    void
    PostTask(
        const Callable& Job
    )
    {
        auto dispatcher = ThreadQueue;
        PostTaskToDispatcher(dispatcher, Job);
    }

    void
    GlobalDispatcherWait(void)
    {
        using namespace std::chrono_literals;
        g_DispatcherMutex.lock();
        while (g_Dispatchers.size() != 0)
        {
            for (auto dispatcher = g_Dispatchers.begin(); dispatcher != g_Dispatchers.end(); dispatcher++)
            {
                if (std::get<1>(*dispatcher)->Completed())
                {
                    g_Dispatchers.erase(dispatcher);
                    break;
                }
            }
            g_DispatcherMutex.unlock();
            std::this_thread::sleep_for(200ms);
        }
    }

}