#include "emberx/runtime/tensor_validation.h"
#include "tests/support/checks.h"

#include <limits>

int main()
{
    Checks checks;
    emberx::AllocationInfo allocation{{1}, 0, 64, true};
    EmberxTensorDesc tensor{};
    tensor.allocation = {1};
    tensor.dtype = EMBERX_DTYPE_FLOAT32;
    tensor.rank = 2;
    tensor.shape[0] = 2;
    tensor.shape[1] = 3;
    tensor.strides[0] = 3;
    tensor.strides[1] = 1;

    checks.expect(!emberx::validate_tensor(tensor, allocation, 0), "2x3 FP32 tensor");
    auto rejects = [&](auto changed, auto info, EmberxStatus code,
                       std::string_view field)
    {
        const auto error = emberx::validate_tensor(changed, info, 0);
        checks.expect(error && error->code == code && error->field == field, field);
    };
    auto changed = tensor;
    changed.byte_offset = 4;
    checks.expect(!emberx::validate_tensor(changed, allocation, 0),
                  "view offset need not match 64-byte allocation alignment");
    changed.byte_offset = 2;
    rejects(changed, allocation, EMBERX_INVALID_ARGUMENT, "byte_offset");
    changed.byte_offset = 44;
    rejects(changed, allocation, EMBERX_INVALID_ARGUMENT, "bounds");

    changed = tensor;
    changed.strides[0] = 4;
    rejects(changed, allocation, EMBERX_UNSUPPORTED_OPERATION, "strides");
    changed = tensor;
    changed.rank = 9;
    rejects(changed, allocation, EMBERX_INVALID_ARGUMENT, "rank");
    changed = tensor;
    changed.dtype = EMBERX_DTYPE_INVALID;
    rejects(changed, allocation, EMBERX_UNSUPPORTED_OPERATION, "dtype");
    changed = tensor;
    changed.allocation = {0};
    rejects(changed, allocation, EMBERX_INVALID_ALLOCATION, "allocation");

    auto info = allocation;
    info.live = false;
    rejects(tensor, info, EMBERX_INVALID_ALLOCATION, "allocation");
    info = allocation;
    info.handle = {2};
    rejects(tensor, info, EMBERX_INVALID_ALLOCATION, "allocation");
    info = allocation;
    info.device = 1;
    rejects(tensor, info, EMBERX_INVALID_DEVICE, "device");

    changed = tensor;
    changed.rank = 0;
    checks.expect(!emberx::validate_tensor(changed, allocation, 0), "scalar needs 4 bytes");
    info = allocation;
    info.size_bytes = 3;
    rejects(changed, info, EMBERX_INVALID_ARGUMENT, "bounds");

    changed = tensor;
    changed.shape[0] = 0;
    changed.byte_offset = 64;
    checks.expect(!emberx::validate_tensor(changed, allocation, 0),
                  "empty tensor at allocation end");
    changed.byte_offset = 68;
    rejects(changed, allocation, EMBERX_INVALID_ARGUMENT, "byte_offset");

    changed = tensor;
    changed.shape[0] = std::numeric_limits<std::uint64_t>::max();
    changed.shape[1] = 2;
    changed.strides[0] = 2;
    rejects(changed, allocation, EMBERX_INVALID_ARGUMENT, "shape");
    changed.rank = 1;
    changed.shape[0] = std::numeric_limits<std::uint64_t>::max() / 4 + 1;
    changed.strides[0] = 1;
    rejects(changed, allocation, EMBERX_INVALID_ARGUMENT, "shape");

    changed = tensor;
    changed.rank = EMBERX_MAX_TENSOR_RANK;
    for (std::uint32_t i = 0; i < changed.rank; i++)
    {
        changed.shape[i] = 1;
        changed.strides[i] = 1;
    }
    checks.expect(!emberx::validate_tensor(changed, allocation, 0), "maximum rank");
    return checks.result();
}
