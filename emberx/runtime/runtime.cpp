#include "emberx/runtime/runtime.h"
#include "emberx/runtime/config/config.h"
#include "emberx/runtime/device.h"

#include <utility>
#include <vector>

namespace emberx {

struct Runtime::State {
    SystemConfig config;
    std::vector<std::unique_ptr<EmberXDevice>> devices;
};

Runtime::Runtime() = default;
Runtime::~Runtime() = default;

ValidationError Runtime::initialize(const std::filesystem::path& config_path) {
    if (config_path.empty())
        return Error{EMBERX_INVALID_ARGUMENT, "config_path", "configuration path is empty"};

    // Serialize the whole transaction, including repeated reads and validation.
    std::lock_guard lock(mutex_);
    auto loaded = load_config(config_path);
    if (const auto* error = std::get_if<Error>(&loaded))
        return *error;
    auto& config = std::get<SystemConfig>(loaded);

    if (state_) {
        if (state_->config == config)
            return std::nullopt;
        return Error{EMBERX_ALREADY_INITIALIZED, "configuration",
                     "runtime is initialized with a different configuration"};
    }

    auto candidate = std::make_unique<State>();
    candidate->config = std::move(config);
    candidate->devices.reserve(candidate->config.devices.size());
    for (const auto& device : candidate->config.devices)
        candidate->devices.push_back(std::make_unique<EmberXDevice>(device));

    // The only publication point cannot throw. Any earlier exception destroys
    // the candidate and leaves the runtime uninitialized.
    state_.swap(candidate);
    return std::nullopt;
}

Result<std::uint32_t> Runtime::device_count() const {
    std::lock_guard lock(mutex_);
    if (!state_)
        return Error{EMBERX_NOT_INITIALIZED, "runtime", "call emberxInit first"};
    // Configuration validation has already limited the inventory to 256 entries.
    return static_cast<std::uint32_t>(state_->devices.size());
}

Result<EmberxDeviceProperties> Runtime::device_properties(EmberxDeviceId device) const {
    std::lock_guard lock(mutex_);
    if (!state_)
        return Error{EMBERX_NOT_INITIALIZED, "runtime", "call emberxInit first"};
    if (device >= state_->devices.size())
        return Error{EMBERX_INVALID_DEVICE, "device", "ID is outside the configured inventory"};
    return state_->devices[device]->properties();
}

ValidationError Runtime::validate_device(EmberxDeviceId device) const {
    std::lock_guard lock(mutex_);
    if (!state_)
        return Error{EMBERX_NOT_INITIALIZED, "runtime", "call emberxInit first"};
    if (device >= state_->devices.size())
        return Error{EMBERX_INVALID_DEVICE, "device", "ID is outside the configured inventory"};
    return std::nullopt;
}

} // namespace emberx
