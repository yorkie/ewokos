#ifndef MMU_DEF_H
#define MMU_DEF_H

#include <stdint.h>

#define KB 1024
#define MB (1024*KB)
#define GB (1024*MB)

#define PAGE_SIZE 4096
#define PAGE_DIR_SIZE 4096
#define PAGE_DIR_NUM (PAGE_DIR_SIZE/4)
#define PAGE_TABLE_SIZE 4096

#define AP_RW_D 0
#define AP_RW_R 1
#define AP_RW_W 2
#define AP_RW_RW 3

#define PTE_ATTR_WRBACK 0
#define PTE_ATTR_DEV 1
#define PTE_ATTR_WRTHR 2
#define PTE_ATTR_WRBACK_ALLOCATE 3
#define PTE_ATTR_STRONG_ORDER 4
#define PTE_ATTR_NOCACHE 5

#define INTERRUPT_VECTOR_BASE 0xffff0000

#define WASM_PTE_PRESENT 0x001u
#define WASM_PTE_WRITE   0x002u
#define WASM_PTE_USER    0x004u
#define WASM_PTE_ADDR_MASK 0xfffff000u

#endif
