#pragma once

#include "emberx/tensor.h"

#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

namespace emberx
{

    struct DeviceBuffer
    {
        EmberxDeviceId device;
        EmberxAllocationHandle allocation;
        std::uint64_t byte_offset;
    };
    struct HostSource
    {
        const std::byte *data;
        std::uint64_t size_bytes;
    };
    struct HostDestination
    {
        std::byte *data;
        std::uint64_t size_bytes;
    };
    struct CopyCommand
    {
        std::variant<HostSource, DeviceBuffer> source;
        std::variant<HostDestination, DeviceBuffer> destination;
        std::uint64_t bytes;
    };

    enum class KernelId
    {
        Add,
        Matmul,
        Rmsnorm,
        Silu
    };
    struct KernelCommand
    {
        KernelId kernel;
        std::vector<EmberxTensorDesc> inputs;
        std::vector<EmberxTensorDesc> outputs;
        std::vector<float> scalars;
    };
    struct EventRecordCommand
    {
        EmberxEventHandle event;
    };
    struct EventWaitCommand
    {
        EmberxEventHandle event;
    };
    struct SendCommand
    {
        EmberxDeviceId peer;
        std::uint64_t message_id;
        DeviceBuffer source;
        std::uint64_t bytes;
    };
    struct RecvCommand
    {
        EmberxDeviceId peer;
        std::uint64_t message_id;
        DeviceBuffer destination;
        std::uint64_t bytes;
    };

    using Command = std::variant<CopyCommand, KernelCommand, EventRecordCommand,
                                 EventWaitCommand, SendCommand, RecvCommand>;

    struct Submission
    {
        EmberxDeviceId device;
        EmberxStreamHandle stream;
        Command command;
    };

} // namespace emberx