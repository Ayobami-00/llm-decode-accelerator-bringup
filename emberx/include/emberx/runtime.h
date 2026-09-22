#ifndef EMBERX_RUNTIME_H
#define EMBERX_RUNTIME_H

#include "emberx/status.h"
#include "emberx/types.h"

/* Capability flags have their own bit assignments, independent of dtype IDs. */
#define EMBERX_DTYPE_FLAG_FLOAT32 UINT32_C(1)

/* Caller-owned snapshot of configured capabilities, not execution measurements.
 * Capacities/offset alignments are bytes. Counts do not imply throughput.
 * Coordinates are placement labels, not implicit fabric connectivity.
 */
typedef struct EmberxDeviceProperties {
    EmberxDeviceId id;
    uint64_t sram_bytes;
    uint64_t allocation_alignment_bytes;
    uint32_t supported_dtype_flags;
    uint32_t matrix_engines;
    uint32_t vector_engines;
    uint32_t dma_engines;
    uint32_t fabric_link_count;
    uint32_t topology_coordinates[2];
} EmberxDeviceProperties;

#ifdef __cplusplus
extern "C" {
#endif

/* Read and validate config_path on every call; null/empty paths are invalid.
 * First success publishes the complete logical inventory, without allocating SRAM.
 * Equivalent typed configurations succeed without resetting devices or selections.
 * Different valid configurations return EMBERX_ALREADY_INITIALIZED. Any failure
 * preserves prior state; a failed first initialization can be retried.
 * The inventory lives until process exit; no reset/shutdown API is provided.
 */
EmberxStatus emberxInit(const char* config_path);

/* All getters require writable, nonnull output pointers and leave outputs
 * unchanged on failure. Checks run in this order: required pointer, runtime
 * initialization, then device ID. No function initializes the runtime implicitly.
 * Functions are safe to call from multiple threads with separate output storage.
 * Concurrent calls may wait for initialization. C++ exceptions never escape:
 * allocation failures return OUT_OF_MEMORY; other unexpected failures INTERNAL_ERROR.
 */
EmberxStatus emberxGetDeviceCount(uint32_t* count);

/* Queries an explicit ID without changing the calling thread's selection.
 * Modifying the returned snapshot cannot change runtime-owned properties.
 */
EmberxStatus emberxGetDeviceProperties(
    EmberxDeviceId device, EmberxDeviceProperties* properties);

/* Selection is local to the calling host thread. Each thread defaults to zero;
 * a new thread does not inherit its creator's selection. IDs must be below the
 * actual configured count. Failed selection preserves the previous value.
 */
EmberxStatus emberxSetDevice(EmberxDeviceId device);
EmberxStatus emberxGetDevice(EmberxDeviceId* device);

#ifdef __cplusplus
}
#endif

#endif /* EMBERX_RUNTIME_H */
