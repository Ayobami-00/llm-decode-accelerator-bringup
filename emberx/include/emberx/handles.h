#ifndef EMBERX_HANDLES_H
#define EMBERX_HANDLES_H

#include <stdint.h>

/* Zero is invalid. Distinct struct types prevent accidental handle mixing. */
typedef struct EmberxAllocationHandle { uint64_t value; } EmberxAllocationHandle;
typedef struct EmberxStreamHandle { uint64_t value; } EmberxStreamHandle;
typedef struct EmberxEventHandle { uint64_t value; } EmberxEventHandle;

#endif
