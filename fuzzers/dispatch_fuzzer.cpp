#include <fuzzer/FuzzedDataProvider.h>

#include <chrono>
#include <iostream>

#include "SharedRefptr.hpp"
#include "DispatchQueue.hpp"

enum class Action
{
    kPostTask,
    kPostTasks,
    kPostDelayedTask,
    kPostOther,
    kEnd,
    kMaxValue
};

void
PerformJob(
    SharedPtr<FuzzedDataProvider> Fdp,
    dispatch::DispatcherBasePtr Second
)
{
    if (Fdp->remaining_bytes() < 5)
    {
        dispatch::End();
        return;
    }

    // std::cout << dispatch::CurrentQueue()->Size() << ' ' << Fdp->remaining_bytes() << std::endl;

    auto action = Fdp->ConsumeEnum<Action>();
    size_t count;

    switch (action)
    {
    case Action::kMaxValue:
    case Action::kPostTask:
        dispatch::PostTask(
            dispatch::bind(
                &PerformJob,
                Fdp,
                Second
            )
        );
        break;
    case Action::kPostOther:
        if (dispatch::CurrentQueue() == Second.get())
        {
            dispatch::PostTaskToDispatcher(
                "Main",
                dispatch::bind(
                    &PerformJob,
                    Fdp,
                    Second
                )
            );
        }
        else
        {
            Second->PostTask(
                dispatch::bind(
                    &PerformJob,
                    Fdp,
                    Second
                )
            );
        }
        dispatch::PostTask(
            dispatch::bind(
                &PerformJob,
                Fdp,
                Second
            )
        );
        break;
    case Action::kPostTasks:
        count = Fdp->ConsumeIntegralInRange<size_t>(1, 200);
        for (size_t i = 0; i < count; i++)
        {
            dispatch::PostTask(
                dispatch::bind(
                    &PerformJob,
                    Fdp,
                    Second
                )
            );
        }
        break;
    case Action::kPostDelayedTask:
        dispatch::PostDelayedTask(
            dispatch::bind(
                &PerformJob,
                Fdp,
                Second
            ),
            std::chrono::milliseconds(25)
        );
        break;
    case Action::kEnd:
        dispatch::End();
        break;
    }

}

extern "C" int
LLVMFuzzerTestOneInput(
    const uint8_t *Data,
    size_t Size
)
{
    auto fdp = MakeShared<FuzzedDataProvider>(Data, Size);
    auto second = dispatch::CreateDispatcher("Second");
    dispatch::CreateAndEnterDispatcher(
        "Main",
        dispatch::bind(
            &PerformJob,
            fdp,
            second
        )
    );
    second->Stop();
    second->Wait();
    return 0;
}