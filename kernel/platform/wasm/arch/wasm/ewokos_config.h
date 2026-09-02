#ifndef EWOKOS_CONFIG_H
#define EWOKOS_CONFIG_H

#define KERNEL_BASE 0x30000000u
#define RAM_BASE    0x0
#define MAX_USABLE_MEM_SIZE (128u * 1024u * 1024u)

/* The browser links every native Wasm process into one shared linear-memory
 * address space. Keep a 32 MiB kernel virtual-allocation window and make the
 * otherwise unused upper half available for application modules. */
#define KMALLOC_VM_SIZE 0x02000000u

#define EWOK_STACK_ALIGN      16u
#define EWOK_STACK_INIT_BIAS  0u

#include <stdint.h>

typedef uint32_t ewokos_addr_t;

#endif
