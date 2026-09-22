function(check_example label expected_code expected_text)
    execute_process(
        COMMAND "${PROGRAM}" ${ARGN}
        RESULT_VARIABLE code
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if(NOT "${code}" STREQUAL "${expected_code}")
        message(FATAL_ERROR "${label}: exit ${code}; expected ${expected_code}\n${output}${error}")
    endif()
    string(FIND "${output}${error}" "${expected_text}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "${label}: missing '${expected_text}'\n${output}${error}")
    endif()
endfunction()

check_example(single 0 "Selected device: 0\nCurrent device: 0" "${SINGLE}")
check_example(multiple 0 "Selected device: 1\nCurrent device: 1" "${MULTIPLE}")
check_example(properties 0
    "Device 1:\n  SRAM: 2097152 bytes\n  Allocation alignment: 64 bytes\n  Dtypes: float32\n  Matrix/vector/DMA engines: 1/1/1\n  Fabric links: 0\n  Coordinates: [1, 0]"
    "${MULTIPLE}")
check_example(explicit-selection 0 "Selected device: 0\nCurrent device: 0" "${MULTIPLE}" 0)
check_example(absent-device 1 "EMBERX_INVALID_DEVICE" "${MULTIPLE}" 2)
foreach(argument IN ITEMS -1 +1 1x 4294967296)
    check_example(invalid-argument 1 "device_id must be an unsigned decimal uint32_t value"
        "${MULTIPLE}" "${argument}")
endforeach()
check_example(usage 1 "Usage: emberx_device_info")
check_example(invalid-configuration 1 "EMBERX_INVALID_CONFIGURATION" "${INVALID}")
# A child of the known regular file cannot exist as a configuration file.
check_example(missing-configuration 1 "EMBERX_IO_ERROR" "${SINGLE}/missing.yaml")
