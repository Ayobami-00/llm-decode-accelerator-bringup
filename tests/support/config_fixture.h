#pragma once

#include "emberx/runtime/config/config.h"

inline emberx::SystemConfig valid_config()
{
    emberx::DeviceConfig device{};
    device.id = 0;
    device.memory = {1'048'576, 64};
    device.supported_dtypes = {EMBERX_DTYPE_FLOAT32};
    device.engines = {1, 1, 1};
    device.topology.coordinates = {0, 0};

    emberx::SystemConfig config{};
    config.schema_version = 1;
    config.architecture = "emberx-v0";
    config.devices.push_back(device);
    return config;
}