#include <chrono>
#include <iostream>
#include <string>

#include "DispatchQueue.hpp"

void
Test(
    const std::string Argument
)
{
    std::cout << Argument << std::endl;
}

void
DoEnd(
    void
)
{
    std::cout << "End" << std::endl;
    dispatch::End();
}

int main()
{
    //
    // Create the dispatchers
    //
    auto d2 = dispatch::CreateDispatcher(
        "main"
    );

    d2->PostDelayedTask(
        dispatch::bind(
            &Test,
            "Hello three seconds"
        ),
        std::chrono::seconds(3)
    );
    d2->PostTask(
        dispatch::bind(
            &Test,
            "Immediate"
        )
    );
    d2->PostDelayedTask(
        dispatch::bind(
            &Test,
            "Hello four seconds"
        ),
        std::chrono::seconds(4)
    );
    d2->PostDelayedTask(
        dispatch::bind(
            &Test,
            "Hello one second"
        ),
        std::chrono::seconds(1)
    );
    d2->PostDelayedTask(
        dispatch::bind(
            &DoEnd
        ),
        std::chrono::seconds(6)
    );
    
    d2->Wait();

}