#include <iostream>
#include <thread>
#include "DispatchQueue.hpp"

#define PRIMARY "primary"
#define TASK1 "First Task"
#define TASK2 "Second Task"

void Test(const std::string Argument)
{
    std::cout << '[' << std::this_thread::get_id() << "] " << Argument << std::endl;
    if (Argument == TASK1){
        dispatch::PostTask(
            std::bind(&Test, TASK2)
        );
    }
}

int main(){
#ifdef DEBUG
    std::cout << "Running DEUBG build" << std::endl;
#endif
    //
    // Create an anonymous dispatcher
    //
    (void) dispatch::CreateDispatcher();

    //
    // Create the primary dispatcher
    //
    auto dispatcher = dispatch::CreateDispatcher(
        PRIMARY,
        dispatch::bind(&Test, TASK1)
    );
    // dispatcher->Wait();
    std::cout << "End of Main Thread" << std::endl;
}