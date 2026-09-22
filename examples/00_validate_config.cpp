#include "emberx/runtime/config/config.h"

#include <cstdlib>
#include <iostream>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: emberx_validate_config <configuration.yaml>\n";
        return EXIT_FAILURE;
    }

    auto result = emberx::load_config(argv[1]);
    if (const auto *error = std::get_if<emberx::Error>(&result))
    {
        std::cerr << "Error " << error->code << " at " << error->field
                  << ": " << error->message << '\n';
        return EXIT_FAILURE;
    }

    const auto &config = std::get<emberx::SystemConfig>(result);
    std::cout << "Architecture: " << config.architecture
              << "\nDevices: " << config.devices.size() << '\n';
    for (const auto &device : config.devices)
    {
        std::cout << "Device " << device.id << ":\n"
                  << "  SRAM: " << device.memory.sram_bytes << " bytes\n"
                  << "  Allocation alignment: "
                  << device.memory.allocation_alignment_bytes << " bytes\n"
                  << "  Dtypes: float32\n"
                  << "  Matrix/vector/DMA engines: " << device.engines.matrix
                  << '/' << device.engines.vector << '/' << device.engines.dma
                  << '\n';
    }
    return EXIT_SUCCESS;
}