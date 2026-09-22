#include "emberx/runtime/runtime.h"
#include "emberx/runtime/device.h"
#include "tests/support/checks.h"
#include "tests/support/config_fixture.h"
#include "tests/support/runtime_fixture.h"

#include <limits>

namespace {

template <typename T>
bool is_error(const emberx::Result<T>& result, EmberxStatus status) {
    const auto* error = std::get_if<emberx::Error>(&result);
    return error && error->code == status;
}

void check_count(Checks& checks, const emberx::Runtime& runtime, uint32_t expected) {
    const auto result = runtime.device_count();
    const auto* count = std::get_if<uint32_t>(&result);
    checks.expect(count && *count == expected, "registered device count");
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4)
        return EXIT_FAILURE;
    Checks checks;
    RuntimeFixtureDirectory files;
    const auto malformed = files.write("malformed.yaml", "devices: [");
    const auto equivalent = files.write("equivalent.yaml", device_config_text(2, true));

    const auto baseline = valid_config();
    checks.expect(baseline == valid_config(), "configuration has value equality");
    auto differs = [&](auto change) {
        auto config = baseline;
        change(config);
        checks.expect(config != baseline, "configuration equality includes changed field");
    };
    differs([](auto& c) { c.schema_version++; });
    differs([](auto& c) { c.architecture += "-other"; });
    differs([](auto& c) { c.devices.push_back(c.devices.front()); });
    differs([](auto& c) { c.devices[0].id++; });
    differs([](auto& c) { c.devices[0].memory.sram_bytes++; });
    differs([](auto& c) { c.devices[0].memory.allocation_alignment_bytes++; });
    differs([](auto& c) { c.devices[0].supported_dtypes = {EMBERX_DTYPE_INVALID}; });
    differs([](auto& c) { c.devices[0].engines.matrix++; });
    differs([](auto& c) { c.devices[0].engines.vector++; });
    differs([](auto& c) { c.devices[0].engines.dma++; });
    differs([](auto& c) { c.devices[0].topology.coordinates[0]++; });
    differs([](auto& c) { c.devices[0].topology.coordinates[1]++; });
    differs([](auto& c) { c.fabric_links.push_back({}); });
    auto ordered = baseline;
    ordered.devices.push_back(ordered.devices.front());
    ordered.devices[1].id = 1;
    auto reversed = ordered;
    std::swap(reversed.devices[0], reversed.devices[1]);
    checks.expect(ordered != reversed, "device order participates in equality");

    auto device_config = baseline.devices.front();
    emberx::EmberXDevice device(device_config);
    device_config.memory.sram_bytes = 0;
    auto snapshot = device.properties();
    snapshot.sram_bytes = 7;
    checks.expect(expected_properties(device.properties(), 0),
                  "device owns its metadata and returns independent snapshots");

    emberx::Runtime runtime;
    checks.expect(is_error(runtime.device_count(), EMBERX_NOT_INITIALIZED), "count before init");
    checks.expect(is_error(runtime.device_properties(999), EMBERX_NOT_INITIALIZED),
                  "initialization precedes ID validation");
    auto error = runtime.validate_device(0);
    checks.expect(error && error->code == EMBERX_NOT_INITIALIZED, "validation before init");
    error = runtime.initialize({});
    checks.expect(error && error->code == EMBERX_INVALID_ARGUMENT, "empty path");
    error = runtime.initialize(files.missing());
    checks.expect(error && error->code == EMBERX_IO_ERROR, "missing configuration");
    error = runtime.initialize(malformed);
    checks.expect(error && error->code == EMBERX_YAML_SYNTAX_ERROR, "malformed configuration");
    error = runtime.initialize(argv[3]);
    checks.expect(error && error->code == EMBERX_INVALID_CONFIGURATION, "invalid configuration");
    checks.expect(is_error(runtime.device_count(), EMBERX_NOT_INITIALIZED),
                  "failed initialization publishes no inventory");

    checks.expect(!runtime.initialize(argv[1]), "retry with valid single-device configuration");
    check_count(checks, runtime, 1);
    checks.expect(!runtime.initialize(argv[1]), "repeated identical configuration succeeds");
    checks.expect(!runtime.validate_device(0), "configured device is valid");
    checks.expect(is_error(runtime.device_properties(1), EMBERX_INVALID_DEVICE),
                  "ID below format limit can still be absent");

    emberx::Runtime multiple;
    checks.expect(!multiple.initialize(argv[2]), "independent runtime with two devices");
    for (uint32_t id = 0; id < 2; id++) {
        const auto result = multiple.device_properties(id);
        const auto* properties = std::get_if<EmberxDeviceProperties>(&result);
        checks.expect(properties && expected_properties(*properties, id), "distinct complete properties");
    }
    checks.expect(!multiple.initialize(equivalent), "different file and spelling with equal values");
    error = multiple.initialize(argv[1]);
    checks.expect(error && error->code == EMBERX_ALREADY_INITIALIZED, "conflicting inventory rejected");
    error = multiple.initialize(files.missing());
    checks.expect(error && error->code == EMBERX_IO_ERROR, "repeat still reads the file");
    error = multiple.initialize(argv[3]);
    checks.expect(error && error->code == EMBERX_INVALID_CONFIGURATION, "repeat still validates");
    check_count(checks, multiple, 2);
    check_count(checks, runtime, 1);

    const auto changing = files.write("changing.yaml", device_config_text(2));
    emberx::Runtime edited_file;
    checks.expect(!edited_file.initialize(changing), "initial file contents accepted");
    files.write("changing.yaml", device_config_text(1));
    error = edited_file.initialize(changing);
    checks.expect(error && error->code == EMBERX_ALREADY_INITIALIZED,
                  "same filename with different contents conflicts");
    check_count(checks, edited_file, 2);

    // Registration stores capacity metadata; it must not try to back SRAM yet.
    auto huge = device_config_text(1);
    huge.replace(huge.find("1048576"), 7,
                 std::to_string(std::numeric_limits<uint64_t>::max() - 63));
    emberx::Runtime large_capacity;
    checks.expect(!large_capacity.initialize(files.write("huge.yaml", huge)),
                  "large representable capacity requires no SRAM allocation");
    const auto large = large_capacity.device_properties(0);
    const auto* properties = std::get_if<EmberxDeviceProperties>(&large);
    checks.expect(properties && properties->sram_bytes == std::numeric_limits<uint64_t>::max() - 63,
                  "capacity is not narrowed or silently reduced");
    return checks.result();
}
