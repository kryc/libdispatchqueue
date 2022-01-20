#pragma once

#include <functional>

namespace dispatch{

    using Callable = std::function<void(void)>;

    template<class... Args>
    Callable bind(Args&&... x)
    /*++
      std::bind wrapper to return a formal Callable
    --*/
    { 
        return Callable(
            std::bind(
                std::forward<Args>(x)...
            )
        );
    }

    template<typename Fn, class... Args>
    Fn bindf(Args&&... x)
    /*++
      Utility wrapper to create a std::function
      callback.
      
      Usage:
        using Cb = std::function<int(int)>;
        int Test(int Arg){...}
        ...
        dispatch::bindf<Cb>(&Test, std::placeholders::_1)
    --*/
    { 
        return Fn(
            std::bind(
                std::forward<Args>(x)...
            )
        );
    }

}