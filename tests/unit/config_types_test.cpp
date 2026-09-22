#include "tests/support/checks.h"
#include "tests/support/config_fixture.h"

int main()
{
    Checks checks;
    const auto config = valid_config();
    checks.expect(config.devices.size() == 1, "construct one device without YAML");
    checks.expect(config.devices[0].memory.sram_bytes == 1'048'576,
                  "retain byte count");
    checks.expect(config.devices[0].supported_dtypes[0] == EMBERX_DTYPE_FLOAT32,
                  "store dtype as an enum");
    return checks.result();
}