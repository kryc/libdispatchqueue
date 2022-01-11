#pragma once

#include <functional>

using Callable = std::function<void()>;

template<class... Args>
Callable bind(Args&&... x)
{ 
    return Callable(
        std::bind(
            std::forward<Args>(x)...
        )
    );
}