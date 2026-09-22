#include "emberx/runtime.h"
#include "emberx/runtime/runtime.h"

#include <new>

namespace {

emberx::Runtime& runtime() {
    static emberx::Runtime instance;
    return instance;
}

thread_local EmberxDeviceId current_device = 0;

template <typename Function>
EmberxStatus api_call(Function&& function) noexcept {
    try {
        return function();
    } catch (const std::bad_alloc&) {
        return EMBERX_OUT_OF_MEMORY;
    } catch (...) {
        return EMBERX_INTERNAL_ERROR;
    }
}

template <typename T>
EmberxStatus write_result(const emberx::Result<T>& result, T* output) {
    if (const auto* error = std::get_if<emberx::Error>(&result))
        return error->code;
    *output = std::get<T>(result);
    return EMBERX_SUCCESS;
}

} // namespace

extern "C" {

EmberxStatus emberxInit(const char* config_path) {
    if (!config_path || !*config_path)
        return EMBERX_INVALID_ARGUMENT;
    return api_call([&] {
        const auto error = runtime().initialize(config_path);
        return error ? error->code : EMBERX_SUCCESS;
    });
}

EmberxStatus emberxGetDeviceCount(uint32_t* count) {
    if (!count)
        return EMBERX_INVALID_ARGUMENT;
    return api_call([&] { return write_result(runtime().device_count(), count); });
}

EmberxStatus emberxGetDeviceProperties(
    EmberxDeviceId device, EmberxDeviceProperties* properties) {
    if (!properties)
        return EMBERX_INVALID_ARGUMENT;
    return api_call([&] {
        return write_result(runtime().device_properties(device), properties);
    });
}

EmberxStatus emberxSetDevice(EmberxDeviceId device) {
    return api_call([&] {
        if (const auto error = runtime().validate_device(device))
            return error->code;
        current_device = device;
        return EMBERX_SUCCESS;
    });
}

EmberxStatus emberxGetDevice(EmberxDeviceId* device) {
    if (!device)
        return EMBERX_INVALID_ARGUMENT;
    return api_call([&] {
        if (const auto error = runtime().validate_device(current_device))
            return error->code;
        *device = current_device;
        return EMBERX_SUCCESS;
    });
}

} // extern "C"
