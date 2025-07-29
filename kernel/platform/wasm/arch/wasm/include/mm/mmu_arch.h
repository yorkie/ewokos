#ifndef MMU_ARCH_H
#define MMU_ARCH_H

#include "mmudef.h"

// WASM doesn't have traditional MMU, so we simplify these structures

#define PAGE_DIR_INDEX(x) ((uint32_t)x >> 20)
#define PAGE_INDEX(x) (((uint32_t)x >> 12) & 255)

#define PAGE_TABLE_TO_BASE(x) ((uint32_t)x >> 10)
#define BASE_TO_PAGE_TABLE(x) ((void *) ((uint32_t)x << 10))
#define PAGE_TO_BASE(x) ((uint32_t)x >> 12)

/* Simplified page directory entry for WASM */
typedef struct {
	uint32_t base;
} page_dir_entry_t;

/* Simplified page table entry for WASM */
typedef struct {
	uint32_t base;
} page_table_entry_t; 

/* V5 page table entry - not used in WASM but needed for compilation */
typedef struct {
	uint32_t base;
} page_table_entry_v5_t; 

// WASM doesn't need PTE flags, but provide stubs
static inline void set_pte_flags(page_table_entry_t* pte, uint32_t pte_attr) {
    (void)pte; (void)pte_attr; // no-op
}

#define PTE_ATTR_WRBACK          0
#define PTE_ATTR_DEV             1
#define PTE_ATTR_WRTHR           2
#define PTE_ATTR_WRBACK_ALLOCATE 3
#define PTE_ATTR_STRONG_ORDER    4
#define PTE_ATTR_NOCACHE         5

// Function declarations - these will be implemented as stubs in WASM
int32_t  map_page(page_dir_entry_t *vm, 
	ewokos_addr_t virtual_addr, 
	ewokos_addr_t physical,
	uint32_t access_permissions, 
	uint32_t pte_attr);

void unmap_page(page_dir_entry_t *vm, ewokos_addr_t virtual_addr);

ewokos_addr_t resolve_phy_address(page_dir_entry_t *vm, ewokos_addr_t virtual);
page_table_entry_t* get_page_table_entry(page_dir_entry_t *vm, ewokos_addr_t virtual);
void free_page_tables(page_dir_entry_t *vm);

void __set_translation_table_base(ewokos_addr_t);
void __flush_tlb(void);

#endif