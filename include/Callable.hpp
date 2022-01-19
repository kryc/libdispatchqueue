#pragma once

#include <functional>

namespace dispatch{

    using Callable = std::function<void(void)>;

    template<class... Args>
    Callable bind(Args&&... x)
    { 
        return Callable(
            std::bind(
                std::forward<Args>(x)...
            )
        );
    }

}