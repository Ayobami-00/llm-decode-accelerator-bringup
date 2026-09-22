#include "emberx/runtime/config/config.h"
#include "emberx/contract.h"
#include <bit>
#include <set>

namespace emberx
{

    ValidationError validate_config(const SystemConfig &config)
    {
        auto fail = [](std::string field, std::string message) -> ValidationError
        {
            return Error{EMBERX_INVALID_CONFIGURATION, field, message};
        };

        if (config.schema_version != emberxGetContractSchemaVersion())
            return fail("schema_version", "expected schema version 1");
        if (config.architecture != kArchitecture)
            return fail("architecture", "expected emberx-v0");
        if (config.devices.empty() || config.devices.size() > kMaxDevices)
            return fail("devices", "expected between 1 and 256 devices");
        if (!config.fabric_links.empty())
            return fail("fabric.links", "only an empty link list is supported");

        std::set<std::array<std::uint32_t, 2>> coordinates;
        for (std::size_t i = 0; i < config.devices.size(); i++)
        {
            const auto &device = config.devices[i];
            const std::string path = "devices[" + std::to_string(i) + "]";
            if (device.id != i)
                return fail(path + ".id", "ID must equal its list position");

            const auto &memory = device.memory;
            const auto alignment = memory.allocation_alignment_bytes;
            if (memory.sram_bytes == 0)
                return fail(path + ".memory.sram_bytes", "capacity must be positive");
            if (alignment < 4 || !std::has_single_bit(alignment))
                return fail(path + ".memory.allocation_alignment_bytes",
                            "alignment must be a power of two of at least 4");
            if (alignment > memory.sram_bytes)
                return fail(path + ".memory.allocation_alignment_bytes",
                            "alignment cannot exceed capacity");
            if (memory.sram_bytes % alignment != 0)
                return fail(path + ".memory.sram_bytes",
                            "capacity must be a multiple of alignment");

            if (device.supported_dtypes.size() != 1 ||
                device.supported_dtypes.front() != EMBERX_DTYPE_FLOAT32)
                return fail(path + ".supported_dtypes", "expected only float32");
            if (device.engines.matrix != 1)
                return fail(path + ".engines.matrix", "expected one matrix engine");
            if (device.engines.vector != 1)
                return fail(path + ".engines.vector", "expected one vector engine");
            if (device.engines.dma != 1)
                return fail(path + ".engines.dma", "expected one DMA engine");
            if (!coordinates.insert(device.topology.coordinates).second)
                return fail(path + ".topology.coordinates",
                            "coordinate pair must be unique");
        }
        return std::nullopt;
    }

} // namespace emberx