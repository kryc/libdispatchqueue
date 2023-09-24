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
    // Basic shared_refptr tests
    //
    dispatch::SharedRefPtr<std::string> ptr;

    // operator bool()
    assert(!ptr);
    assert(ptr.get_manager() == nullptr);
    assert(ptr.get() == nullptr);
    assert(ptr.refs() == 0);

    ptr = dispatch::MakeSharedRefPtr<std::string>("TestPtr");

    assert(ptr);
    assert(ptr.get_manager() != nullptr);
    assert(ptr.get() != nullptr);
    assert(ptr.refs() == 1);

    auto ptr_copy = ptr;

    assert(ptr.refs() == 2);

    auto ptr_copy_2 = ptr_copy;

    assert(ptr.refs() == 3);

    auto ptr_move = std::move(ptr_copy_2);

    assert(ptr_move.get_manager() != nullptr);
    assert(ptr_move.get() != nullptr);
    assert(ptr_move.refs() == 3);

    assert(ptr_copy_2.get_manager() == nullptr);
    assert(ptr_copy_2.get() == nullptr);
    assert(ptr_copy_2.refs() == 0);

    ptr_move = nullptr;
    ptr_copy = nullptr;

    assert(ptr.get_manager() != nullptr);
    assert(ptr.get() != nullptr);
    assert(ptr.refs() == 1);

    assert(ptr_copy_2.get_manager() == nullptr);
    assert(ptr_copy_2.get() == nullptr);
    assert(ptr_copy_2.refs() == 0);

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