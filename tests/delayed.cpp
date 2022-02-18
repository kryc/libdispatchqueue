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
            "Hello"
        ),
        std::chrono::microseconds(10)
    );

}