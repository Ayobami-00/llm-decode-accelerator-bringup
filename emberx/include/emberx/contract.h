#ifndef EMBERX_CONTRACT_H
#define EMBERX_CONTRACT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the configuration schema version supported by the draft contract. */
uint32_t emberxGetContractSchemaVersion(void);

#ifdef __cplusplus
}
#endif

#endif /* EMBERX_CONTRACT_H */
