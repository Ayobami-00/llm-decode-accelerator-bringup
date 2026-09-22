#include "emberx/runtime/tensor_validation.h"
#include "emberx/runtime/config/config.h"

#include <limits>

namespace emberx {

ValidationError validate_tensor(const EmberxTensorDesc& tensor,
                                const AllocationInfo& allocation,
                                EmberxDeviceId expected_device) {
    auto fail = [](EmberxStatus code, std::string field,
                   std::string message) -> ValidationError {
        return Error{code, field, message};
    };
    if (expected_device >= kMaxDevices || allocation.device != expected_device)
        return fail(EMBERX_INVALID_DEVICE, "device", "allocation belongs to another device");
    if (!allocation.live || tensor.allocation.value == 0 ||
        tensor.allocation.value != allocation.handle.value)
        return fail(EMBERX_INVALID_ALLOCATION, "allocation", "allocation is not live or does not match");
    if (tensor.dtype != EMBERX_DTYPE_FLOAT32)
        return fail(EMBERX_UNSUPPORTED_OPERATION, "dtype", "only float32 is supported");
    if (tensor.rank > EMBERX_MAX_TENSOR_RANK)
        return fail(EMBERX_INVALID_ARGUMENT, "rank", "maximum rank is 8");
    if (tensor.byte_offset % 4 != 0)
        return fail(EMBERX_INVALID_ARGUMENT, "byte_offset", "FP32 offsets must be multiples of 4");
    if (tensor.byte_offset > allocation.size_bytes)
        return fail(EMBERX_INVALID_ARGUMENT, "byte_offset", "offset exceeds allocation size");

    // Zero-sized tensors access no elements; their strides are ignored.
    for (std::uint32_t i = 0; i < tensor.rank; i++) {
        if (tensor.shape[i] == 0)
            return std::nullopt;
    }

    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t elements = 1; // A rank-zero tensor is one scalar.
    for (std::uint32_t i = tensor.rank; i > 0; --i) {
        if (tensor.strides[i - 1] != elements)
            return fail(EMBERX_UNSUPPORTED_OPERATION, "strides", "expected canonical row-major strides");
        if (tensor.shape[i - 1] > maximum / elements)
            return fail(EMBERX_INVALID_ARGUMENT, "shape", "element count overflows uint64_t");
        elements *= tensor.shape[i - 1];
    }
    if (elements > maximum / 4)
        return fail(EMBERX_INVALID_ARGUMENT, "shape", "byte count overflows uint64_t");
    const auto required_bytes = elements * 4;
    // Subtract after checking the offset; avoid overflowing offset + size.
    if (required_bytes > allocation.size_bytes - tensor.byte_offset)
        return fail(EMBERX_INVALID_ARGUMENT, "bounds", "tensor extends past the allocation");
    return std::nullopt;
}

} // namespace emberx
