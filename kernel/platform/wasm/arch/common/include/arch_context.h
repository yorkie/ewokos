#ifndef ARCH_CONTEXT_H
#define ARCH_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>
#include <ewokos_config.h>

// WASM simplified context structure
typedef struct {
    ewokos_addr_t pc;      // Program counter
    ewokos_addr_t sp;      // Stack pointer
    ewokos_addr_t lr;      // Link register (return address)
    ewokos_addr_t gpr[16]; // General purpose registers
    uint32_t cpsr;         // Current program status register (simulated)
} context_t;

#define CONTEXT_INIT(x) (x.cpsr = 0x50)

#endif