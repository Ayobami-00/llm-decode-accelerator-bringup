# llm-decode-accelerator-bringup
Pre-silicon bring-up of EmberX, an experimental LLM decode accelerator: simulation, runtime and PyTorch integration, distributed execution, KV-cache management, and serving integrations with vLLM, SGLang, and NVIDIA Dynamo.

## Current state

Implemented:

- YAML-independent C++ configuration types and structured errors.
- Strict YAML text/file loading and configuration validation for `emberx-v0`.
- C-compatible handles and tensor descriptors, with pure metadata, layout,
  bounds, and overflow validation using supplied allocation records.
- Command payloads, an abstract backend interface, and a test-only fake that
  demonstrates acceptance and completion-state reporting.
- A configuration-inspection example and six registered tests.

Device initialization, the allocator and resource registry, scheduling, actual
copies, kernels, and a functional simulator remain future work. FIFO ordering,
dependency propagation, and buffer lifetime enforcement are specified but have
not been implemented or tested. PyTorch and serving integrations are later
project milestones.

Design notes and the implementation guide are maintained locally in the ignored
`docs` directory and are not versioned with the source. The article is maintained
outside this repository.

## Build and test

Requirements: CMake 3.24 or newer, a C++20 compiler, Git, and a build tool such as
Make or Ninja. The first configuration needs network access to fetch yaml-cpp.

From the repository root:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The build creates the static `emberx_contract` library, the
`emberx_validate_config` example, and six test executables. Expected CTest result:
**6 tests pass**, including the original schema-version smoke test.

| Registered test | Coverage |
| --- | --- |
| `emberx_contract_smoke` | Public schema-version function linked from the library |
| `emberx_config_types_tests` | Direct C++ configuration construction without YAML |
| `emberx_config_tests` | Semantic configuration rules and limits |
| `emberx_yaml_loader_tests` | Valid loading, malformed input, schema errors, and file I/O errors |
| `emberx_tensor_tests` | Allocation metadata, tensor layout, bounds, and overflow |
| `emberx_backend_contract_tests` | Test-only acceptance and completion states |

Checks use ordinary C++ conditionals rather than `assert`, so they remain active
in Release builds. The concrete fake backend exists only inside its test file.

yaml-cpp 0.8.0 is a private library dependency, pinned to commit
`f7320141120f720aecc4c32be25586e7da9eb978` through CMake FetchContent. Its own tests
and tools are disabled. C++20 is required and compiler-specific language
extensions are disabled.

CMake also exports compiler settings to `build/compile_commands.json` for
Make/Ninja builds. An editor can use this file to match C++20 and the actual
include paths. Local `.vscode` settings are ignored and must be configured
separately on a fresh checkout.

## Inspect a configuration

From the repository root:

```sh
./build/emberx_validate_config simulator/configs/single-device.yaml
```

Expected output:

```text
Architecture: emberx-v0
Devices: 1
Device 0:
  SRAM: 1048576 bytes
  Allocation alignment: 64 bytes
  Dtypes: float32
  Matrix/vector/DMA engines: 1/1/1
```

This loads and validates metadata; it does not create a device or allocate SRAM.
Capacity and engine counts are bring-up settings, with no performance assumptions.

These deliberate failure checks should each return a nonzero exit status:

```sh
./build/emberx_validate_config tests/fixtures/config/invalid-alignment.yaml
echo $?
./build/emberx_validate_config tests/fixtures/config/does-not-exist.yaml
echo $?
```

The first reports `Error 3` (`EMBERX_INVALID_CONFIGURATION`) at
`devices[0].memory.allocation_alignment_bytes`. The second reports `Error 1`
(`EMBERX_IO_ERROR`) with the missing filename. These expected CLI failures are
acceptance checks, not failed unit tests.

## Build without tests

To build the library and inspection example with tests disabled:

```sh
cmake -S . -B build-no-tests -DBUILD_TESTING=OFF
cmake --build build-no-tests
ctest --test-dir build-no-tests -N
./build-no-tests/emberx_validate_config simulator/configs/single-device.yaml
```

Expect zero registered tests in this separate build directory. The inspection
example remains available independently of the test suite. Generated files
stay in the ignored build directories.
