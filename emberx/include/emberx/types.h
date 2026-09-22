#ifndef EMBERX_TYPES_H
#define EMBERX_TYPES_H

#include <stdint.h>

typedef uint32_t EmberxDeviceId;

typedef enum EmberxDType
{
    EMBERX_DTYPE_INVALID = 0,
    EMBERX_DTYPE_FLOAT32 = 1
} EmberxDType;

#endif
