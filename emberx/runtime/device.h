#pragma once

#include "emberx/runtime.h"
#include "emberx/runtime/config/config.h"

namespace emberx {

// Owns one device's immutable metadata. The config must come from a validated
// SystemConfig; actual memory and execution resources are later work.
class EmberXDevice {
public:
    explicit EmberXDevice(const DeviceConfig& config);
    EmberxDeviceProperties properties() const noexcept { return properties_; }

private:
    const EmberxDeviceProperties properties_;
};

} // namespace emberx
