#include "emberx/contract.h"
#include "emberx/handles.h"
#include "emberx/runtime.h"
#include "emberx/tensor.h"

#include <stdlib.h>

int main(int argc, char** argv) {
    uint32_t count = 99;
    EmberxDeviceId selected = 99;
    EmberxDeviceProperties properties = {0};
    EmberxTensorDesc tensor = {0};
    if (argc != 2 || tensor.rank != 0 || emberxGetContractSchemaVersion() != 1)
        return EXIT_FAILURE;
    if (emberxGetDeviceCount(&count) != EMBERX_NOT_INITIALIZED || count != 99)
        return EXIT_FAILURE;
    if (emberxInit(argv[1]) != EMBERX_SUCCESS ||
        emberxGetDeviceCount(&count) != EMBERX_SUCCESS || count != 1 ||
        emberxGetDeviceProperties(0, &properties) != EMBERX_SUCCESS ||
        properties.id != 0 || properties.sram_bytes != UINT64_C(1048576) ||
        properties.supported_dtype_flags != EMBERX_DTYPE_FLAG_FLOAT32 ||
        emberxSetDevice(0) != EMBERX_SUCCESS ||
        emberxGetDevice(&selected) != EMBERX_SUCCESS || selected != 0)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
