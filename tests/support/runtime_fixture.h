#pragma once

#include "emberx/runtime.h"

#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

// Every test process owns a unique temporary directory, including parallel runs.
class RuntimeFixtureDirectory {
public:
    RuntimeFixtureDirectory() {
        std::random_device random;
        for (unsigned attempt = 0; attempt < 100; attempt++) {
            path_ = std::filesystem::temp_directory_path() /
                ("emberx-runtime-" + std::to_string(random()) + "-" + std::to_string(attempt));
            if (std::filesystem::create_directory(path_))
                return;
        }
        throw std::runtime_error("could not create runtime fixture directory");
    }
    ~RuntimeFixtureDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }
    RuntimeFixtureDirectory(const RuntimeFixtureDirectory&) = delete;
    RuntimeFixtureDirectory& operator=(const RuntimeFixtureDirectory&) = delete;

    std::filesystem::path write(std::string_view name, std::string_view text) const {
        const auto destination = path_ / name;
        std::ofstream output;
        output.exceptions(std::ios::failbit | std::ios::badbit);
        output.open(destination);
        output << text;
        output.close();
        return destination;
    }
    std::filesystem::path missing() const { return path_ / "missing.yaml"; }

private:
    std::filesystem::path path_;
};

inline std::string device_config_text(uint32_t count, bool leading_zeroes = false) {
    std::ostringstream yaml;
    yaml << "# Generated configuration fixture\nschema_version: 1\n"
            "architecture: emberx-v0\ndevices:\n";
    for (uint32_t id = 0; id < count; id++) {
        yaml << "  - id: " << id << "\n    memory:\n      sram_bytes: "
             << (uint64_t{id} + 1) * 1'048'576
             << "\n      allocation_alignment_bytes: " << (leading_zeroes ? "064" : "64")
             << "\n    supported_dtypes: [float32]\n"
                "    engines: {matrix: 1, vector: 1, dma: 1}\n"
                "    topology: {coordinates: [" << id << ", 0]}\n";
    }
    yaml << "fabric: {links: []}\n";
    return yaml.str();
}

inline bool same_properties(const EmberxDeviceProperties& left,
                            const EmberxDeviceProperties& right) {
    // Compare fields, not padding bytes in the C-compatible struct.
    return left.id == right.id && left.sram_bytes == right.sram_bytes &&
        left.allocation_alignment_bytes == right.allocation_alignment_bytes &&
        left.supported_dtype_flags == right.supported_dtype_flags &&
        left.matrix_engines == right.matrix_engines &&
        left.vector_engines == right.vector_engines && left.dma_engines == right.dma_engines &&
        left.fabric_link_count == right.fabric_link_count &&
        left.topology_coordinates[0] == right.topology_coordinates[0] &&
        left.topology_coordinates[1] == right.topology_coordinates[1];
}

inline bool expected_properties(const EmberxDeviceProperties& properties, EmberxDeviceId id) {
    const EmberxDeviceProperties expected{
        id, (uint64_t{id} + 1) * 1'048'576, 64, EMBERX_DTYPE_FLAG_FLOAT32,
        1, 1, 1, 0, {id, 0}};
    return same_properties(properties, expected);
}
