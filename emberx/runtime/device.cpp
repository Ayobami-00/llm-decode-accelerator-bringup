#include "emberx/runtime/device.h"

namespace emberx {

EmberXDevice::EmberXDevice(const DeviceConfig& config)
    : properties_{
          config.id,
          config.memory.sram_bytes,
          config.memory.allocation_alignment_bytes,
          // The validated emberx-v0 profile requires exactly float32 and no links.
          EMBERX_DTYPE_FLAG_FLOAT32,
          config.engines.matrix,
          config.engines.vector,
          config.engines.dma,
          0,
          {config.topology.coordinates[0], config.topology.coordinates[1]}} {}

} // namespace emberx
