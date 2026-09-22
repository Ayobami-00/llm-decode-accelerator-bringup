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

add_executable(emberx_validate_config examples/00_validate_config.cpp)
target_include_directories(emberx_validate_config PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(emberx_validate_config PRIVATE emberx_contract)

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
endif()
