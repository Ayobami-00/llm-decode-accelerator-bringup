#pragma once

#include "emberx/runtime.h"
#include "emberx/runtime/result.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>

namespace emberx {

// This class owns the inventory, not thread-local selection. Independent
// instances let internal tests exercise initialization without a public reset.
class Runtime {
public:
    Runtime();
    ~Runtime();
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    ValidationError initialize(const std::filesystem::path& config_path);
    Result<std::uint32_t> device_count() const;
    Result<EmberxDeviceProperties> device_properties(EmberxDeviceId device) const;
    ValidationError validate_device(EmberxDeviceId device) const;

private:
    struct State;
    mutable std::mutex mutex_;
    std::unique_ptr<State> state_;
};

} // namespace emberx
