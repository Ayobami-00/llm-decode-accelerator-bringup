#pragma once

#include "emberx/types.h"
#include "emberx/runtime/result.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace emberx {

inline constexpr std::uint32_t kMaxDevices = 256;
inline constexpr std::string_view kArchitecture = "emberx-v0";

struct MemoryConfig {
    std::uint64_t sram_bytes{};
    std::uint64_t allocation_alignment_bytes{};
    bool operator==(const MemoryConfig&) const = default;
};

struct EngineConfig {
    std::uint32_t matrix{};
    std::uint32_t vector{};
    std::uint32_t dma{};
    bool operator==(const EngineConfig&) const = default;
};

struct TopologyConfig {
    std::array<std::uint32_t, 2> coordinates{};
    bool operator==(const TopologyConfig&) const = default;
};

struct DeviceConfig {
    EmberxDeviceId id{};
    MemoryConfig memory{};
    std::vector<EmberxDType> supported_dtypes;
    EngineConfig engines{};
    TopologyConfig topology{};
    bool operator==(const DeviceConfig&) const = default;
};

// No link payload is defined yet: every nonempty fabric list is rejected.
struct FabricLinkConfig {
    bool operator==(const FabricLinkConfig&) const = default;
};

struct SystemConfig {
    std::uint32_t schema_version{};
    std::string architecture;
    std::vector<DeviceConfig> devices;
    std::vector<FabricLinkConfig> fabric_links;
    bool operator==(const SystemConfig&) const = default;
};

ValidationError validate_config(const SystemConfig& config);
Result<SystemConfig> load_config_text(std::string_view text);
Result<SystemConfig> load_config(const std::filesystem::path& path);

} // namespace emberx
