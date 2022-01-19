#include <assert.h>
#include <iostream>
#include <thread>
#include "DispatchQueue.hpp"

#define PRIMARY "primary"
#define SECONDARY "secondary"
#define TASK1 "First Task"
#define TASK2 "Second Task"
#define TASK3 "Third Task"
#define TASK4 "Fourth Task"
#define TASK5 "Fifth Task"
#define TASK6 "Sixth Task"
#define LASTTASK "Last Task"

void Test(const std::string Argument)
{
    std::cout << '[' << std::this_thread::get_id() << "] " << Argument << std::endl;
    if (Argument == TASK1){
        dispatch::PostTask(
            std::bind(&Test, TASK2)
        );
    }
    else if (Argument == TASK2)
    {
        dispatch::PostTaskToDispatcher(
            SECONDARY,
            std::bind(&Test, TASK3)
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
            std::bind(&Test, TASK5),
            std::bind(&Test, LASTTASK)
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
    // Create the primary dispatcher
    //
    auto dispatcher = dispatch::CreateDispatcher(
        PRIMARY,
        dispatch::bind(&Test, TASK1)
    );
    
    dispatch::GlobalDispatcherWait();

    std::cout << "End of Main Thread" << std::endl;
}