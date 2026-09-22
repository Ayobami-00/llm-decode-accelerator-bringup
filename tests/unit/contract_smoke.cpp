#include "emberx/contract.h"

#include <cstdlib>
#include <iostream>

int main() {
    const uint32_t expected_schema_version = 1;
    const uint32_t actual_schema_version = emberxGetContractSchemaVersion();

    // An explicit check also runs in Release builds, where assert may be disabled.
    if (actual_schema_version != expected_schema_version) {
        std::cerr << "Expected schema version " << expected_schema_version
                  << ", received " << actual_schema_version << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "EmberX contract smoke test passed: schema version "
              << actual_schema_version << '\n';
    return EXIT_SUCCESS;
}
