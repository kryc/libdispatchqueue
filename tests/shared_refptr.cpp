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
}

void
DoEnd(
    void
)
{
    dispatch::End();
}

void
DoBound(
    dispatch::BoundRefPtr<std::string> Ptr
)
{
    std::cout << Ptr->c_str() << std::endl << *Ptr << std::endl;
    dispatch::PostTaskToDispatcher(
        "second",
        dispatch::bind(
            &DoBound,
            Ptr
        )
    );
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
            &DoEnd
        )
    );
}

void
BoundLoop()
{
    auto bptr = dispatch::MakeBoundRefPtr<std::string>("Bound");
    dispatch::PostTask(
        dispatch::bind(
            &DoBound,
            bptr
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

    auto d3 = dispatch::CreateAndEnterDispatcher(
        "third",
        dispatch::bind(
            &BoundLoop
        )
    );
}