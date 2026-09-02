#ifndef ARCH_CONTEXT_H
#define ARCH_CONTEXT_H

#include <stdint.h>
#include "ewokos_config.h"

typedef struct {
    uint32_t cpsr;
    uint32_t pc;
    uint32_t gpr[13];
    uint32_t sp;
    uint32_t lr;
} context_t;

#define CONTEXT_INIT(c) \
    do { \
        (c).cpsr = 0; \
        (c).pc = 0; \
        (c).sp = 0; \
        (c).lr = 0; \
    } while(0)

#endif
