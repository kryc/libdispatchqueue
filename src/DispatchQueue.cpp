#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <mutex>
#include <algorithm>

#include "DispatchQueue.hpp"

namespace dispatch
{
    std::mutex g_DispatcherMutex;
    std::map<std::string, std::shared_ptr<DispatcherBase>> g_Dispatchers;
    thread_local DispatcherBase* ThreadQueue = nullptr;

    DispatcherBase*
    CurrentQueue(void)
    {
        return ThreadQueue;
    }

    std::shared_ptr<DispatcherBase>
    CreateDispatcher(std::string Name, Callable& Entrypoint)
    {
        std::lock_guard<std::mutex> mutex(g_DispatcherMutex);
        auto queue = std::make_shared<DispatcherBase>(std::ref(Entrypoint));
        g_Dispatchers[Name] = queue;
        queue->Run();
        return queue;
    }

    std::shared_ptr<DispatcherBase>
    CreateDispatcher(std::string Name)
    {
        std::lock_guard<std::mutex> mutex(g_DispatcherMutex);
        auto queue = std::make_shared<DispatcherBase>();
        g_Dispatchers[Name] = queue;
        queue->Run();
        return queue;
    }

    std::shared_ptr<DispatcherBase>
    GetDispatcher(std::string Name)
    {
        std::lock_guard<std::mutex> mutex(g_DispatcherMutex);
        return g_Dispatchers.at(Name);
    }

    void
    PostTaskToDispatcher(std::string Name, const Callable& Job)
    {
        auto dispatcher = GetDispatcher(Name);
        dispatcher->PostTask(Job);
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