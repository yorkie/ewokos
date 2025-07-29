#ifndef ARCH_CONTEXT_H
#define ARCH_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>
#include <ewokos_config.h>

// Simplified context for WASM - no real registers to save
typedef struct {
    ewokos_addr_t pc;      // Program counter (simulated)
    ewokos_addr_t sp;      // Stack pointer (simulated)  
    ewokos_addr_t lr;      // Link register (simulated)
    uint32_t status;       // Status flags
} context_t;

#define CONTEXT_INIT(x) (x.status = 0)

#endif