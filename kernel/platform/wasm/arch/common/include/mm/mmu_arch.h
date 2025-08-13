#ifndef MMU_ARCH_H
#define MMU_ARCH_H

#include "mmudef.h"

// WASM uses simplified page management since it has flat memory model
#define PAGE_DIR_INDEX(x) ((uint32_t)x >> 20)
#define PAGE_INDEX(x) (((uint32_t)x >> 12) & 255)

#define PAGE_TABLE_TO_BASE(x) ((uint32_t)x >> 10)
#define BASE_TO_PAGE_TABLE(x) ((void *) ((uint32_t)x << 10))
#define PAGE_TO_BASE(x) ((uint32_t)x >> 12)

/* Simplified page directory entry for WASM */
typedef struct {
    uint32_t type   : 2; // 0: fault, 0x1: page table, 0x2: section
    uint32_t sbz    : 3; // should be zero
    uint32_t domain : 4;
    uint32_t p      : 1; // ECC Enable (ignored in WASM)
    uint32_t base   : 22;
} page_dir_entry_t;

/* Simplified page table entry for WASM */
typedef struct {
    uint32_t type       : 2; // 0: fault, 0x2: small size(4k)
    uint32_t writeback  : 1; // B (ignored in WASM)
    uint32_t cacheable  : 1; // C (ignored in WASM)
    uint32_t ap         : 2; // Access Permissions
    uint32_t tex        : 3; // Type Extension Field (ignored in WASM)
    uint32_t apx        : 1; // Access Permissions Extension (ignored in WASM)
    uint32_t sharable   : 1; // (ignored in WASM)
    uint32_t ng         : 1; // Not-Global (ignored in WASM)
    uint32_t base       : 20;
} page_table_entry_t;

// WASM-specific attribute definitions (simplified)
#define PTE_ATTR_WRBACK          0
#define PTE_ATTR_DEV             1
#define PTE_ATTR_WRTHR           2
#define PTE_ATTR_WRBACK_ALLOCATE 3
#define PTE_ATTR_STRONG_ORDER    4
#define PTE_ATTR_NOCACHE         5

// Function declarations
void set_pte_flags(page_table_entry_t* pte, uint32_t pte_attr);

int32_t map_page(page_dir_entry_t *vm, 
    ewokos_addr_t virtual_addr, 
    ewokos_addr_t physical,
    uint32_t access_permissions, 
    uint32_t pte_attr);

void unmap_page(page_dir_entry_t *vm, ewokos_addr_t virtual_addr);

ewokos_addr_t resolve_phy_address(page_dir_entry_t *vm, ewokos_addr_t virtual);
page_table_entry_t* get_page_table_entry(page_dir_entry_t *vm, ewokos_addr_t virtual);
void free_page_tables(page_dir_entry_t *vm);

// WASM-specific functions (no-op implementations)
void __set_translation_table_base(ewokos_addr_t base);
void __flush_tlb(void);

#endif