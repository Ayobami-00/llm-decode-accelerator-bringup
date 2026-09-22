#include "emberx/runtime.h"
#include "tests/support/checks.h"
#include "tests/support/runtime_fixture.h"

#include <array>
#include <barrier>
#include <limits>
#include <thread>

namespace {

EmberxStatus initialize(const std::filesystem::path& path) {
    return emberxInit(path.string().c_str());
}

void expect_current(Checks& checks, EmberxDeviceId expected) {
    EmberxDeviceId current = UINT32_MAX;
    checks.expect(emberxGetDevice(&current) == EMBERX_SUCCESS && current == expected,
                  "current device matches expected thread selection");
}

void expect_inventory(Checks& checks, uint32_t expected) {
    uint32_t count = 0;
    checks.expect(emberxGetDeviceCount(&count) == EMBERX_SUCCESS && count == expected,
                  "inventory count");
    for (uint32_t id = 0; id < expected; id++) {
        EmberxDeviceProperties properties{};
        checks.expect(emberxGetDeviceProperties(id, &properties) == EMBERX_SUCCESS &&
                      expected_properties(properties, id), "all configured properties");
    }
}

void before_initialization(Checks& checks) {
    checks.expect(emberxInit(nullptr) == EMBERX_INVALID_ARGUMENT, "null configuration path");
    checks.expect(emberxInit("") == EMBERX_INVALID_ARGUMENT, "empty configuration path");
    checks.expect(emberxGetDeviceCount(nullptr) == EMBERX_INVALID_ARGUMENT, "null count first");
    checks.expect(emberxGetDevice(nullptr) == EMBERX_INVALID_ARGUMENT, "null current device first");
    checks.expect(emberxGetDeviceProperties(UINT32_MAX, nullptr) == EMBERX_INVALID_ARGUMENT,
                  "null properties before initialization and ID checks");
    uint32_t count = 73;
    EmberxDeviceId current = 74;
    const EmberxDeviceProperties sentinel{75, 76, 77, 78, 79, 80, 81, 82, {83, 84}};
    auto properties = sentinel;
    checks.expect(emberxGetDeviceCount(&count) == EMBERX_NOT_INITIALIZED && count == 73,
                  "count remains unchanged before initialization");
    checks.expect(emberxGetDevice(&current) == EMBERX_NOT_INITIALIZED && current == 74,
                  "current output remains unchanged before initialization");
    checks.expect(emberxGetDeviceProperties(UINT32_MAX, &properties) == EMBERX_NOT_INITIALIZED &&
                  same_properties(properties, sentinel), "properties unchanged; initialization before ID");
    checks.expect(emberxSetDevice(UINT32_MAX) == EMBERX_NOT_INITIALIZED, "set does not implicitly init");
}

void invalid_selection(Checks& checks, uint32_t count, EmberxDeviceId previous) {
    for (EmberxDeviceId invalid : {count, UINT32_MAX}) {
        checks.expect(emberxSetDevice(invalid) == EMBERX_INVALID_DEVICE, "invalid selection rejected");
        expect_current(checks, previous);
        const EmberxDeviceProperties sentinel{11, 12, 13, 14, 15, 16, 17, 18, {19, 20}};
        auto properties = sentinel;
        checks.expect(emberxGetDeviceProperties(invalid, &properties) == EMBERX_INVALID_DEVICE &&
                      same_properties(properties, sentinel), "invalid query leaves every output field unchanged");
    }
    checks.expect(emberxGetDeviceProperties(UINT32_MAX, nullptr) == EMBERX_INVALID_ARGUMENT,
                  "null properties still take precedence after initialization");
    checks.expect(emberxGetDeviceCount(nullptr) == EMBERX_INVALID_ARGUMENT, "null initialized count");
    checks.expect(emberxGetDevice(nullptr) == EMBERX_INVALID_ARGUMENT, "null initialized current");
}

void selection_threads(Checks& checks) {
    checks.expect(emberxSetDevice(1) == EMBERX_SUCCESS, "creator selects device one");
    struct Observation {
        EmberxStatus initial_status{}, set_status{}, final_status{};
        EmberxDeviceId initial = UINT32_MAX, final = UINT32_MAX;
    };
    std::array<Observation, 2> observations{};
    std::barrier synchronize(3);
    auto worker = [&](EmberxDeviceId id) {
        auto& result = observations[id];
        result.initial_status = emberxGetDevice(&result.initial);
        result.set_status = emberxSetDevice(id);
        synchronize.arrive_and_wait(); // Both selections have been made.
        synchronize.arrive_and_wait(); // The parent has changed its selection.
        result.final_status = emberxGetDevice(&result.final);
    };
    std::thread first(worker, 0);
    std::thread second(worker, 1);
    synchronize.arrive_and_wait();
    expect_current(checks, 1);
    checks.expect(emberxSetDevice(0) == EMBERX_SUCCESS, "parent changes independently");
    synchronize.arrive_and_wait();
    first.join();
    second.join();
    for (uint32_t id = 0; id < observations.size(); id++) {
        const auto& result = observations[id];
        checks.expect(result.initial_status == EMBERX_SUCCESS && result.initial == 0,
                      "new thread defaults to zero, not its creator's selection");
        checks.expect(result.set_status == EMBERX_SUCCESS &&
                      result.final_status == EMBERX_SUCCESS && result.final == id,
                      "thread selection survives other threads changing theirs");
    }
    expect_current(checks, 0);
}

void concurrent_initialization(Checks& checks, const std::filesystem::path& first_path,
                               const std::filesystem::path& second_path, bool equivalent) {
    std::array<EmberxStatus, 2> statuses{};
    std::barrier start(3);
    auto worker = [&](unsigned index, const std::filesystem::path& path) {
        start.arrive_and_wait();
        statuses[index] = initialize(path);
    };
    std::thread first(worker, 0, first_path);
    std::thread second(worker, 1, second_path);
    start.arrive_and_wait();
    first.join();
    second.join();
    if (equivalent) {
        checks.expect(statuses[0] == EMBERX_SUCCESS && statuses[1] == EMBERX_SUCCESS,
                      "concurrent equivalent initialization both succeed");
        expect_inventory(checks, 2);
    } else {
        checks.expect((statuses[0] == EMBERX_SUCCESS && statuses[1] == EMBERX_ALREADY_INITIALIZED) ||
                      (statuses[1] == EMBERX_SUCCESS && statuses[0] == EMBERX_ALREADY_INITIALIZED),
                      "exactly one conflicting initialization succeeds");
        expect_inventory(checks, statuses[0] == EMBERX_SUCCESS ? 1 : 2);
    }
    expect_current(checks, 0);
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 5)
        return EXIT_FAILURE;
    Checks checks;
    before_initialization(checks);
    RuntimeFixtureDirectory files;
    const std::string_view scenario = argv[1];
    const std::filesystem::path single = argv[2], multiple = argv[3], invalid = argv[4];

    if (scenario == "single") {
        checks.expect(initialize(files.missing()) == EMBERX_IO_ERROR, "failed first init: missing file");
        checks.expect(initialize(files.write("bad.yaml", "devices: [")) == EMBERX_YAML_SYNTAX_ERROR,
                      "failed first init: YAML syntax");
        checks.expect(initialize(invalid) == EMBERX_INVALID_CONFIGURATION, "failed first init: validation");
        uint32_t count = 99;
        checks.expect(emberxGetDeviceCount(&count) == EMBERX_NOT_INITIALIZED && count == 99,
                      "failed attempts leave no inventory");
        checks.expect(initialize(single) == EMBERX_SUCCESS, "retry succeeds");
        expect_inventory(checks, 1);
        expect_current(checks, 0);
        checks.expect(emberxSetDevice(0) == EMBERX_SUCCESS, "select only device");
        invalid_selection(checks, 1, 0);
    } else if (scenario == "multiple") {
        checks.expect(initialize(multiple) == EMBERX_SUCCESS, "initialize two devices");
        expect_inventory(checks, 2);
        expect_current(checks, 0);
        checks.expect(emberxSetDevice(1) == EMBERX_SUCCESS, "select second device");
        EmberxDeviceProperties properties{};
        checks.expect(emberxGetDeviceProperties(0, &properties) == EMBERX_SUCCESS,
                      "query another device");
        expect_current(checks, 1);
        properties.id = 42;
        properties.sram_bytes = 0;
        expect_inventory(checks, 2);
        invalid_selection(checks, 2, 1);
        checks.expect(initialize(multiple) == EMBERX_SUCCESS, "identical initialization succeeds");
        checks.expect(initialize(files.write("equal.yaml", device_config_text(2, true))) == EMBERX_SUCCESS,
                      "typed equality ignores formatting and decimal spelling");
        expect_current(checks, 1);
        checks.expect(initialize(single) == EMBERX_ALREADY_INITIALIZED, "conflicting configuration rejected");
        checks.expect(initialize(files.missing()) == EMBERX_IO_ERROR, "repeated init reads again");
        checks.expect(initialize(invalid) == EMBERX_INVALID_CONFIGURATION, "repeated init validates again");
        checks.expect(emberxInit(nullptr) == EMBERX_INVALID_ARGUMENT, "repeated init validates path");
        expect_inventory(checks, 2);
        expect_current(checks, 1);
    } else if (scenario == "maximum") {
        checks.expect(initialize(files.write("maximum.yaml", device_config_text(256))) == EMBERX_SUCCESS,
                      "register maximum inventory without backing SRAM");
        expect_inventory(checks, 256);
        checks.expect(emberxSetDevice(255) == EMBERX_SUCCESS, "highest configured ID");
        expect_current(checks, 255);
        invalid_selection(checks, 256, 255);
    } else if (scenario == "threads") {
        checks.expect(initialize(multiple) == EMBERX_SUCCESS, "shared inventory for threads");
        selection_threads(checks);
    } else if (scenario == "concurrent-equivalent") {
        concurrent_initialization(checks, multiple,
            files.write("equal.yaml", device_config_text(2, true)), true);
    } else if (scenario == "concurrent-conflicting") {
        concurrent_initialization(checks, single, multiple, false);
    } else {
        checks.expect(false, "unknown API test scenario");
    }
    return checks.result();
}
