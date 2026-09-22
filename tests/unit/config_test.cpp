#include "tests/support/checks.h"
#include "tests/support/config_fixture.h"

#include <limits>

int main() {
    Checks checks;
    checks.expect(!emberx::validate_config(valid_config()), "valid configuration");

    // Each mutation starts from a fresh, valid configuration.
    auto rejects = [&](auto mutate, std::string_view field) {
        auto config = valid_config();
        mutate(config);
        const auto error = emberx::validate_config(config);
        checks.expect(error && error->code == EMBERX_INVALID_CONFIGURATION &&
                      error->field == field, field);
    };

    rejects([](auto& c) { c.schema_version = 2; }, "schema_version");
    rejects([](auto& c) { c.architecture = "other"; }, "architecture");
    rejects([](auto& c) { c.devices.clear(); }, "devices");
    rejects([](auto& c) { c.devices.resize(257); }, "devices");
    rejects([](auto& c) { c.devices[0].id = 1; }, "devices[0].id");
    rejects([](auto& c) { c.devices[0].memory.sram_bytes = 0; },
            "devices[0].memory.sram_bytes");
    for (std::uint64_t alignment : {0, 2, 48, 2'097'152}) {
        rejects([&](auto& c) {
            c.devices[0].memory.allocation_alignment_bytes = alignment;
        }, "devices[0].memory.allocation_alignment_bytes");
    }
    rejects([](auto& c) { c.devices[0].memory.sram_bytes = 65; },
            "devices[0].memory.sram_bytes");
    rejects([](auto& c) { c.devices[0].supported_dtypes.clear(); },
            "devices[0].supported_dtypes");
    rejects([](auto& c) {
        c.devices[0].supported_dtypes.push_back(EMBERX_DTYPE_FLOAT32);
    }, "devices[0].supported_dtypes");
    rejects([](auto& c) {
        c.devices[0].supported_dtypes = {EMBERX_DTYPE_INVALID};
    }, "devices[0].supported_dtypes");
    rejects([](auto& c) { c.devices[0].engines.matrix = 2; },
            "devices[0].engines.matrix");
    rejects([](auto& c) { c.devices[0].engines.vector = 0; },
            "devices[0].engines.vector");
    rejects([](auto& c) { c.devices[0].engines.dma = 2; },
            "devices[0].engines.dma");
    rejects([](auto& c) { c.fabric_links.push_back({}); }, "fabric.links");

    auto many = valid_config();
    const auto prototype = many.devices.front();
    many.devices.clear();
    for (std::uint32_t i = 0; i < emberx::kMaxDevices; i++) {
        auto device = prototype;
        device.id = i;
        device.topology.coordinates = {i, 0};
        many.devices.push_back(device);
    }
    checks.expect(!emberx::validate_config(many), "256 configured devices");
    many.devices[1].id = 0;
    auto error = emberx::validate_config(many);
    checks.expect(error && error->field == "devices[1].id", "duplicate ID");
    many.devices[1].id = 1;
    many.devices[1].topology.coordinates = {0, 0};
    error = emberx::validate_config(many);
    checks.expect(error && error->field == "devices[1].topology.coordinates",
                  "duplicate coordinates");

    auto large = valid_config();
    large.devices[0].memory.sram_bytes =
        std::numeric_limits<std::uint64_t>::max() - 63;
    checks.expect(!emberx::validate_config(large),
                  "large representable capacity is validated without allocation");
    return checks.result();
}
