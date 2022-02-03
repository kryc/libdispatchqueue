#include <assert.h>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include "DispatchQueue.hpp"

#define PRIMARY "primary"
#define SECONDARY "secondary"
#define POOL "pool"
#define TASK1 "First Task"
#define TASK2 "Second Task"
#define TASK3 "Third Task"
#define TASK4 "Fourth Task"
#define TASK5 "Fifth Task"
#define TASK6 "Sixth Task"
#define LASTTASK "Last Task"
#define EXTRATASK "Extra Task"
#define POOLTASK "Pool Task "
#define POOLSUBTASK "Pool Sub Task "

using CallbackProto = std::function<void(std::string)>;

std::mutex g_PrintLock;

void Callback(
    const CallbackProto Callback,
    const std::string Argument
)
{
    Callback(Argument);
}

void Test(const std::string Argument)
{
    {
        std::lock_guard<std::mutex> mutex(g_PrintLock);
        std::cout << std::hex << '[' << std::this_thread::get_id() << "] " << Argument << std::endl;
    }
    if (Argument == TASK1){
        dispatch::PostTask(
            dispatch::bind(&Test, TASK2)
        );
        dispatch::PostTask(
            dispatch::bind(&Test, EXTRATASK)
        );
    }
    else if (Argument == TASK2)
    {
        dispatch::PostTaskToDispatcher(
            SECONDARY,
            dispatch::bind(&Test, TASK3)
        );
    }
    else if (Argument == TASK3)
    {
        dispatch::PostTaskToDispatcher(
            PRIMARY,
            dispatch::bind(&Test, TASK4)
        );
    }
    else if (Argument == TASK4)
    {
        dispatch::PostTaskAndReply(
            SECONDARY,
            dispatch::bind(
                &Callback,
                dispatch::bindf<CallbackProto>(&Test, std::placeholders::_1),
                TASK5),
            dispatch::bind(&Test, LASTTASK)
        );
    }
    else if (Argument == TASK5)
    {
        dispatch::KeepAlive(false);
    }
    else if (Argument == LASTTASK)
    {
        assert(dispatch::OnDispatcher(PRIMARY));
        dispatch::End();
    }
    else if (Argument == POOLTASK "1")
    {
        for (auto i = 0; i < 5; i++)
        {
            std::stringstream ss;
            ss << POOLSUBTASK << i+1;
            dispatch::PostTask(
                dispatch::bind(
                    &Test,
                    ss.str()
                )
            );
        }
    }
}

int main(){
#ifdef DEBUG
    std::cout << "Running DEUBG build" << std::endl;
#endif

    //
    // Create the secondary dispatcher
    //
    auto secondary = dispatch::CreateDispatcher(
        SECONDARY
    );

    //
    // Create and enter the primary dispatcher
    //
    dispatch::CreateAndEnterDispatcher(
        PRIMARY,
        dispatch::bind(&Test, TASK1)
    );

    //
    // Create a dispatch pool
    //
    auto pool = dispatch::CreateDispatchPool(POOL, 4);
    for (size_t i = 0; i < 10; i++)
    {
        std::stringstream ss;
        ss << POOLTASK << i+1;
        pool->PostTask(
            dispatch::bind(&Test, ss.str())
        );
    }

    pool->Stop();

    dispatch::GlobalDispatcherWait();

    std::cout << "End of Main Thread" << std::endl;
}