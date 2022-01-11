#include <iostream>
#include "DispatchQueue.hpp"

void Test()
{
    std::cout << "Test" << std::endl;
}

int main(){
    dispatch::CreateDispatcher("Main");
    dispatch::PostTaskToDispatcher("Main",
        std::bind(&Test)
    );
}