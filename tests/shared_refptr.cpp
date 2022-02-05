#include <iostream>
#include <string>

#include "SharedRefptr.hpp"

void
DoStuff(
    dispatch::SharedRefPtr<std::string> Ptr
)
{
    std::cout << *Ptr.get() << std::endl;
}

int main()
{
    dispatch::SharedRefPtr<std::string> ptr("Hello");
    auto b = ptr;
    DoStuff(b);
    std::cout << *ptr << std::endl;
}