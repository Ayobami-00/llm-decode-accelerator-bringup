#ifndef EMBERX_TENSOR_H
#define EMBERX_TENSOR_H

#include "emberx/handles.h"
#include "emberx/types.h"

#define EMBERX_MAX_TENSOR_RANK 8

typedef struct EmberxTensorDesc {
    EmberxAllocationHandle allocation;
    uint64_t byte_offset;
    EmberxDType dtype;
    uint32_t rank;
    uint64_t shape[EMBERX_MAX_TENSOR_RANK];
    uint64_t strides[EMBERX_MAX_TENSOR_RANK];
} EmberxTensorDesc;

#endif
