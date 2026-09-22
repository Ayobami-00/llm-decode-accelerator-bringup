cmake_minimum_required(VERSION 3.24)

project(EmberX LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Let editors use the same compiler flags and include paths as the build.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
include(FetchContent)

# Build only the dependency library, without its tests, tools, or install rules.
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    # yaml-cpp 0.8.0, pinned to its immutable release commit.
    GIT_TAG f7320141120f720aecc4c32be25586e7da9eb978
    UPDATE_DISCONNECTED TRUE
)
FetchContent_MakeAvailable(yaml-cpp)

add_library(emberx_contract STATIC
    emberx/runtime/config/contract.cpp
    emberx/runtime/config/config.cpp
    emberx/runtime/config/yaml_loader.cpp
    emberx/runtime/tensor_validation.cpp
)
target_include_directories(emberx_contract PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/emberx/include"
)
target_include_directories(emberx_contract PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(emberx_contract PRIVATE yaml-cpp::yaml-cpp)

find_package(Threads REQUIRED)
add_library(emberx_runtime STATIC
    emberx/runtime/device.cpp
    emberx/runtime/runtime.cpp
    emberx/runtime/api.cpp
)
target_include_directories(emberx_runtime PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/emberx/include"
)
target_include_directories(emberx_runtime PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(emberx_runtime PUBLIC emberx_contract PRIVATE Threads::Threads)

add_executable(emberx_validate_config examples/00_validate_config.cpp)
target_include_directories(emberx_validate_config PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(emberx_validate_config PRIVATE emberx_contract)

add_executable(emberx_device_info examples/01_device_info.cpp)
target_link_libraries(emberx_device_info PRIVATE emberx_runtime)

if(BUILD_TESTING)
    add_executable(emberx_contract_tests
        tests/unit/contract_smoke.cpp
    )
    target_link_libraries(emberx_contract_tests PRIVATE emberx_contract)
    add_test(NAME emberx_contract_smoke COMMAND emberx_contract_tests)

    function(add_emberx_contract_test target source)
        add_executable(${target} ${source})
        target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        target_link_libraries(${target} PRIVATE emberx_contract)
        add_test(NAME ${target} COMMAND ${target} ${ARGN})
    endfunction()

    add_emberx_contract_test(emberx_config_types_tests tests/unit/config_types_test.cpp)
    add_emberx_contract_test(emberx_config_tests tests/unit/config_test.cpp)
    add_emberx_contract_test(emberx_yaml_loader_tests tests/unit/yaml_loader_test.cpp
        "${CMAKE_CURRENT_SOURCE_DIR}/simulator/configs/single-device.yaml"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/config/invalid-alignment.yaml")
    add_emberx_contract_test(emberx_tensor_tests tests/unit/tensor_test.cpp)
    add_emberx_contract_test(emberx_backend_contract_tests tests/unit/backend_contract_test.cpp)

    set(EMBERX_SINGLE_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/simulator/configs/single-device.yaml")
    set(EMBERX_TWO_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/simulator/configs/two-devices.yaml")
    set(EMBERX_INVALID_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/config/invalid-alignment.yaml")

    add_executable(emberx_runtime_tests tests/unit/runtime_test.cpp)
    target_include_directories(emberx_runtime_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(emberx_runtime_tests PRIVATE emberx_runtime)
    add_test(NAME emberx_runtime_tests COMMAND emberx_runtime_tests
        "${EMBERX_SINGLE_CONFIG}" "${EMBERX_TWO_CONFIG}" "${EMBERX_INVALID_CONFIG}")

    add_executable(emberx_device_api_tests tests/unit/device_api_test.cpp)
    target_include_directories(emberx_device_api_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(emberx_device_api_tests PRIVATE emberx_runtime Threads::Threads)
    foreach(scenario IN ITEMS single multiple maximum threads concurrent-equivalent concurrent-conflicting)
        add_test(NAME emberx_device_api_${scenario} COMMAND emberx_device_api_tests
            "${scenario}" "${EMBERX_SINGLE_CONFIG}" "${EMBERX_TWO_CONFIG}" "${EMBERX_INVALID_CONFIG}")
        set_tests_properties(emberx_device_api_${scenario} PROPERTIES TIMEOUT 30)
    endforeach()

    # Compile as C11 and link to the C++ runtime, checking the actual public ABI.
    enable_language(C)
    add_executable(emberx_runtime_c_tests tests/unit/runtime_c_test.c)
    set_target_properties(emberx_runtime_c_tests PROPERTIES
        C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF LINKER_LANGUAGE CXX)
    target_link_libraries(emberx_runtime_c_tests PRIVATE emberx_runtime)
    add_test(NAME emberx_runtime_c_tests COMMAND emberx_runtime_c_tests "${EMBERX_SINGLE_CONFIG}")

    add_test(NAME emberx_device_info_cli COMMAND "${CMAKE_COMMAND}"
        "-DPROGRAM=$<TARGET_FILE:emberx_device_info>"
        "-DSINGLE=${EMBERX_SINGLE_CONFIG}"
        "-DMULTIPLE=${EMBERX_TWO_CONFIG}"
        "-DINVALID=${EMBERX_INVALID_CONFIG}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/device_info_test.cmake")
endif()
