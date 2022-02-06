#include <iostream>
#include <string>

#include "DispatchQueue.hpp"
#include "SharedRefptr.hpp"

void
DoStuff(
    dispatch::SharedRefPtr<std::string> Ptr
)
{
    std::cout << Ptr->c_str() << std::endl << *Ptr << std::endl;
    dispatch::End();
}

void
MainLoop()
{
    auto ptr = dispatch::MakeSharedRefPtr<std::string>("Hello");
    dispatch::PostTaskAndReply(
        "second",
        dispatch::bind(
            &DoStuff,
            ptr
        ),
        dispatch::bind(
            &DoStuff,
            ptr
        )
    );
}

int main()
{
    

    //
    // Create the dispatchers
    //
    auto d = dispatch::CreateDispatcher("second");
    auto d2 = dispatch::CreateAndEnterDispatcher(
        "main",
        dispatch::bind(
            &MainLoop
        )
    );
}