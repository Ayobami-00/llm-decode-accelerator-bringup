#pragma once

#include "emberx/runtime/commands/command.h"
#include "emberx/runtime/result.h"

namespace emberx
{

    struct CompletionId
    {
        std::uint64_t value;
    };
    enum class CompletionState
    {
        Pending,
        Succeeded,
        Failed
    };

    struct Completion
    {
        CompletionState state;
        // Present exactly when state is Failed.
        std::optional<Error> error;
    };

    class Backend
    {
    public:
        virtual ~Backend() = default;
        virtual Result<CompletionId> submit(Submission submission) = 0;
        virtual Result<Completion> query(CompletionId id) = 0;
        virtual Result<Completion> wait(CompletionId id) = 0;
    };

} // namespace emberx