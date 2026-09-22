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
- Logical device registration, property discovery, and thread-local device selection
  through a C-compatible runtime API.
- Configuration-inspection and device-inventory examples, with 15 registered tests.

SRAM backing storage, the allocator and allocation/stream/event registries,
scheduling, actual copies, kernels, and a functional simulator remain future work. FIFO ordering,
dependency propagation, and buffer lifetime enforcement are specified but have
not been implemented or tested. PyTorch and serving integrations are later
project milestones.

Design notes and the implementation guide are maintained locally in the ignored
`docs` directory and are not versioned with the source. The article is maintained
outside this repository.

## Build and test

Requirements: CMake 3.24 or newer, a C++20 compiler, Git, and a build tool such as
Make or Ninja. Tests also require a C11 compiler to check the public C interface.
The first configuration needs network access to fetch yaml-cpp.

From the repository root:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The build creates the static `emberx_contract` and `emberx_runtime` libraries,
the `emberx_validate_config` and `emberx_device_info` examples, and the tests.
Expected CTest result: **15 tests pass**, including the original six contract tests.

| Registered test | Coverage |
| --- | --- |
| `emberx_contract_smoke` | Public schema-version function linked from the library |
| `emberx_config_types_tests` | Direct C++ configuration construction without YAML |
| `emberx_config_tests` | Semantic configuration rules and limits |
| `emberx_yaml_loader_tests` | Valid loading, malformed input, schema errors, and file I/O errors |
| `emberx_tensor_tests` | Allocation metadata, tensor layout, bounds, and overflow |
| `emberx_backend_contract_tests` | Test-only acceptance and completion states |
| `emberx_runtime_tests` | Configuration equality, transactional initialization, independent inventories, and owned properties |
| Six `emberx_device_api_*` cases | Single/multiple/256 devices, failure behavior, thread-local selection, and concurrent initialization |
| `emberx_runtime_c_tests` | A C11 caller compiled and linked against the C++ runtime |
| `emberx_device_info_cli` | Inventory output, explicit/default selection, malformed arguments, and configuration failures |

Checks use ordinary C++ conditionals rather than `assert`, so they remain active
in Release builds. The concrete fake backend exists only inside its test file.

yaml-cpp 0.8.0 is a private library dependency, pinned to commit
`f7320141120f720aecc4c32be25586e7da9eb978` through CMake FetchContent. Its own tests
and tools are disabled. C++20 is required and compiler-specific language
extensions are disabled.

`emberx_runtime` links the contract library and the platform thread support found
by CMake. Its public interface is `emberx/runtime.h`; runtime implementation
headers and YAML parser types are not part of that public interface.

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

## Discover and select logical devices

From the repository root:

```sh
./build/emberx_device_info simulator/configs/single-device.yaml
./build/emberx_device_info simulator/configs/two-devices.yaml
./build/emberx_device_info simulator/configs/two-devices.yaml 0
```

The optional final argument selects a device. Without it, the example selects
the last configured device. The two-device invocation prints:

```text
Devices: 2
Device 0:
  SRAM: 1048576 bytes
  Allocation alignment: 64 bytes
  Dtypes: float32
  Matrix/vector/DMA engines: 1/1/1
  Fabric links: 0
  Coordinates: [0, 0]
Device 1:
  SRAM: 2097152 bytes
  Allocation alignment: 64 bytes
  Dtypes: float32
  Matrix/vector/DMA engines: 1/1/1
  Fabric links: 0
  Coordinates: [1, 0]
Current device: 0
Selected device: 1
Current device: 1
```

These properties describe configured capabilities. Registration owns device
metadata without allocating SRAM or creating execution engines. Dtype flags do
not establish kernel coverage; coordinates imply no fabric links. Throughput,
bandwidth, and free-memory measurements are not reported.

Include `emberx/runtime.h` and link `emberx_runtime` to use these functions:

```c
EmberxStatus emberxInit(const char* config_path);
EmberxStatus emberxGetDeviceCount(uint32_t* count);
EmberxStatus emberxGetDeviceProperties(EmberxDeviceId device,
                                       EmberxDeviceProperties* properties);
EmberxStatus emberxSetDevice(EmberxDeviceId device);
EmberxStatus emberxGetDevice(EmberxDeviceId* device);
```

The property snapshot contains device ID, SRAM bytes, allocation alignment in
bytes, dtype flags, matrix/vector/DMA counts, fabric-link count, and two topology
coordinates. `EMBERX_DTYPE_FLAG_FLOAT32` is bit 0, independent of dtype enum IDs.
Snapshots belong to the caller; modifying one cannot change the registered device.

Initialization rules:

- Pass a nonnull, nonempty configuration path. No API initializes implicitly.
- Each call reads and validates the file, including calls after initialization.
- First success publishes the complete inventory. A failed first attempt can be
  retried; parsing, validation, or construction failures preserve prior state.
- Equivalent typed configurations succeed without rebuilding devices or resetting
  selections. Different filenames, comments, formatting, and decimal leading
  zeroes do not affect equivalence.
- A different valid configuration returns `EMBERX_ALREADY_INITIALIZED`. An
  unreadable or invalid repeated input retains its I/O, YAML, or configuration
  error. The active inventory remains usable in either case.
- The process shares one inventory. Initialization and queries synchronize through
  one mutex, so concurrent queries may wait for initialization to complete. The
  inventory remains registered until process exit; there is no reset/shutdown API.

Selection and failure rules:

- Each host thread defaults to device 0. New threads do not inherit the creator's
  selection. `emberxSetDevice` changes only the calling thread.
- `emberxGetDeviceProperties` queries its explicit ID without changing selection.
- IDs must be below the actual configured count. Invalid selection returns
  `EMBERX_INVALID_DEVICE` and preserves the previous selection.
- Getters require writable output pointers and leave outputs unchanged on failure.
  Error precedence is null output pointer (`EMBERX_INVALID_ARGUMENT`), uninitialized
  runtime (`EMBERX_NOT_INITIALIZED`), then invalid device ID.
- C++ exceptions are contained at the public API: allocation failure reports
  `EMBERX_OUT_OF_MEMORY`; unexpected failures report `EMBERX_INTERNAL_ERROR`.

The new statuses are appended to the existing status enum, preserving its earlier
numeric values. Public API tests that need a fresh runtime run in separate CTest
processes; concurrency tests synchronize threads explicitly without timing sleeps.

## Build without tests

To build both libraries and examples with tests disabled:

```sh
cmake -S . -B build-no-tests -DBUILD_TESTING=OFF
cmake --build build-no-tests
ctest --test-dir build-no-tests -N
./build-no-tests/emberx_validate_config simulator/configs/single-device.yaml
./build-no-tests/emberx_device_info simulator/configs/two-devices.yaml
```

Expect zero registered tests in this separate build directory. Both examples
remain available independently of the test suite. Generated files
stay in the ignored build directories.
