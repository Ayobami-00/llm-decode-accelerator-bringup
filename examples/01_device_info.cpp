#include "emberx/runtime.h"

#include <charconv>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

const char* status_name(EmberxStatus status) {
    switch (status) {
    case EMBERX_SUCCESS: return "EMBERX_SUCCESS";
    case EMBERX_IO_ERROR: return "EMBERX_IO_ERROR";
    case EMBERX_YAML_SYNTAX_ERROR: return "EMBERX_YAML_SYNTAX_ERROR";
    case EMBERX_INVALID_CONFIGURATION: return "EMBERX_INVALID_CONFIGURATION";
    case EMBERX_INVALID_ARGUMENT: return "EMBERX_INVALID_ARGUMENT";
    case EMBERX_INVALID_DEVICE: return "EMBERX_INVALID_DEVICE";
    case EMBERX_OUT_OF_MEMORY: return "EMBERX_OUT_OF_MEMORY";
    case EMBERX_NOT_INITIALIZED: return "EMBERX_NOT_INITIALIZED";
    case EMBERX_ALREADY_INITIALIZED: return "EMBERX_ALREADY_INITIALIZED";
    case EMBERX_INTERNAL_ERROR: return "EMBERX_INTERNAL_ERROR";
    default: return "unknown status";
    }
}

bool succeeded(EmberxStatus status, const char* operation) {
    if (status == EMBERX_SUCCESS)
        return true;
    std::cerr << operation << " failed: " << status_name(status)
              << " (" << status << ")\n";
    return false;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) {
        std::cerr << "Usage: emberx_device_info <configuration.yaml> [device_id]\n";
        return EXIT_FAILURE;
    }

    EmberxDeviceId requested = 0;
    if (argc == 3) {
        const std::string_view argument = argv[2];
        const auto parsed = std::from_chars(
            argument.data(), argument.data() + argument.size(), requested, 10);
        if (parsed.ec != std::errc{} || parsed.ptr != argument.data() + argument.size()) {
            std::cerr << "device_id must be an unsigned decimal uint32_t value\n";
            return EXIT_FAILURE;
        }
    }

    if (!succeeded(emberxInit(argv[1]), "emberxInit"))
        return EXIT_FAILURE;

    uint32_t count = 0;
    if (!succeeded(emberxGetDeviceCount(&count), "emberxGetDeviceCount"))
        return EXIT_FAILURE;
    std::cout << "Devices: " << count << '\n';
    for (EmberxDeviceId id = 0; id < count; id++) {
        EmberxDeviceProperties properties{};
        if (!succeeded(emberxGetDeviceProperties(id, &properties), "emberxGetDeviceProperties"))
            return EXIT_FAILURE;
        std::cout << "Device " << properties.id << ":\n"
                  << "  SRAM: " << properties.sram_bytes << " bytes\n"
                  << "  Allocation alignment: " << properties.allocation_alignment_bytes << " bytes\n"
                  << "  Dtypes: "
                  << ((properties.supported_dtype_flags & EMBERX_DTYPE_FLAG_FLOAT32) ? "float32" : "none")
                  << "\n  Matrix/vector/DMA engines: " << properties.matrix_engines
                  << '/' << properties.vector_engines << '/' << properties.dma_engines
                  << "\n  Fabric links: " << properties.fabric_link_count
                  << "\n  Coordinates: [" << properties.topology_coordinates[0]
                  << ", " << properties.topology_coordinates[1] << "]\n";
    }

    EmberxDeviceId current = 0;
    if (!succeeded(emberxGetDevice(&current), "emberxGetDevice"))
        return EXIT_FAILURE;
    std::cout << "Current device: " << current << '\n';
    if (argc == 2)
        requested = count - 1; // Validated configurations contain at least one device.
    if (!succeeded(emberxSetDevice(requested), "emberxSetDevice"))
        return EXIT_FAILURE;
    std::cout << "Selected device: " << requested << '\n';
    if (!succeeded(emberxGetDevice(&current), "emberxGetDevice"))
        return EXIT_FAILURE;
    std::cout << "Current device: " << current << '\n';
    return EXIT_SUCCESS;
}
