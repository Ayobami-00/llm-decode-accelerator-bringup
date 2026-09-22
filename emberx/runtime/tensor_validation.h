#pragma once

#include "emberx/tensor.h"
#include "emberx/runtime/result.h"

#include <cstdint>

namespace emberx {

struct AllocationInfo {
    EmberxAllocationHandle handle;
    EmberxDeviceId device;
    std::uint64_t size_bytes;
    bool live;
};

ValidationError validate_tensor(const EmberxTensorDesc& tensor,
                                const AllocationInfo& allocation,
                                EmberxDeviceId expected_device);

} // namespace emberx