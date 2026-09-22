#include "emberx/backends/backend.h"
#include "tests/support/checks.h"

#include <map>

// Test-only behavior. No queues, memory, transfers, or kernels are implemented.
class FakeBackend final : public emberx::Backend {
public:
    explicit FakeBackend(bool fail_execution = false) : fail_execution_(fail_execution) {}

    emberx::Result<emberx::CompletionId> submit(emberx::Submission request) override {
        if (request.device != 0)
            return emberx::Error{EMBERX_INVALID_DEVICE, "device", "fake has only device 0"};
        if (request.stream.value != 1)
            return emberx::Error{EMBERX_INVALID_STREAM, "stream", "fake has only stream 1"};
        const auto* record = std::get_if<emberx::EventRecordCommand>(&request.command);
        if (!record)
            return emberx::Error{EMBERX_UNSUPPORTED_OPERATION, "command", "fake only accepts event record"};
        if (record->event.value != 1)
            return emberx::Error{EMBERX_INVALID_EVENT, "event", "fake has only event 1"};
        if (!states_.empty())
            return emberx::Error{EMBERX_INVALID_EVENT, "event", "event already recorded"};
        const emberx::CompletionId id{1};
        states_.emplace(id.value, emberx::Completion{emberx::CompletionState::Pending,
                                                   std::nullopt});
        return id;
    }

    emberx::Result<emberx::Completion> query(emberx::CompletionId id) override {
        const auto found = states_.find(id.value);
        if (found == states_.end())
            return emberx::Error{EMBERX_INVALID_COMPLETION, "completion", "unknown ticket"};
        return found->second;
    }

    emberx::Result<emberx::Completion> wait(emberx::CompletionId id) override {
        const auto found = states_.find(id.value);
        if (found == states_.end())
            return emberx::Error{EMBERX_INVALID_COMPLETION, "completion", "unknown ticket"};
        auto& completion = found->second;
        if (completion.state == emberx::CompletionState::Pending) {
            if (fail_execution_) {
                completion = {emberx::CompletionState::Failed,
                    emberx::Error{EMBERX_EXECUTION_FAILED, "command", "injected test failure"}};
            } else {
                completion = {emberx::CompletionState::Succeeded, std::nullopt};
            }
        }
        return completion;
    }

private:
    bool fail_execution_;
    std::map<std::uint64_t, emberx::Completion> states_;
};

int main() {
    Checks checks;
    const emberx::Submission request{
        0, {1}, emberx::EventRecordCommand{EmberxEventHandle{1}}
    };
    auto is_state = [](const auto& result, emberx::CompletionState state) {
        const auto* completion = std::get_if<emberx::Completion>(&result);
        return completion && completion->state == state;
    };

    FakeBackend backend;
    const auto accepted = backend.submit(request);
    const auto* ticket = std::get_if<emberx::CompletionId>(&accepted);
    checks.expect(ticket != nullptr, "submission accepted");
    if (ticket) {
        checks.expect(is_state(backend.query(*ticket), emberx::CompletionState::Pending),
                      "accepted is not completed");
        const auto completed = backend.wait(*ticket);
        checks.expect(is_state(completed, emberx::CompletionState::Succeeded),
                      "wait reaches success");
        const auto* value = std::get_if<emberx::Completion>(&completed);
        checks.expect(value && !value->error, "success has no execution error");
    }

    auto invalid = request;
    invalid.device = 1;
    const auto rejected = backend.submit(invalid);
    const auto* immediate = std::get_if<emberx::Error>(&rejected);
    checks.expect(immediate && immediate->code == EMBERX_INVALID_DEVICE,
                  "submission can fail immediately");

    FakeBackend failing(true);
    const auto failure_ticket = failing.submit(request);
    const auto* pending = std::get_if<emberx::CompletionId>(&failure_ticket);
    checks.expect(pending != nullptr, "later failure is initially accepted");
    if (pending) {
        const auto failed = failing.wait(*pending);
        const auto* completion = std::get_if<emberx::Completion>(&failed);
        checks.expect(completion && completion->state == emberx::CompletionState::Failed &&
                      completion->error &&
                      completion->error->code == EMBERX_EXECUTION_FAILED,
                      "execution failure arrives through completion");
    }
    const auto unknown = backend.query({99});
    const auto* error = std::get_if<emberx::Error>(&unknown);
    checks.expect(error && error->code == EMBERX_INVALID_COMPLETION,
                  "unknown ticket differs from execution failure");
    return checks.result();
}