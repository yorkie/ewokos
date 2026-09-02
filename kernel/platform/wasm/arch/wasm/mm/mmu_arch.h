#ifndef MMU_ARCH_H
#define MMU_ARCH_H

#include "mmudef.h"
#include "ewokos_config.h"

typedef struct {
    uint32_t value;
} page_dir_entry_t;

typedef struct {
    uint32_t value;
} page_table_entry_t;

typedef struct {
    uint32_t paddr;
    uint32_t vaddr;
    uint32_t size;
    uint32_t attr;
} map1_t;

// Function prototypes
void __set_translation_table_base(uint32_t ttb);
void __flush_tlb(void);

int32_t map_page(page_dir_entry_t *vm, uint32_t virtual_addr, uint32_t physical, uint32_t permissions, uint32_t pte_attr);
void unmap_page(page_dir_entry_t *vm, uint32_t virtual_addr);
uint32_t resolve_phy_address(page_dir_entry_t *vm, uint32_t virtual_addr);
page_table_entry_t* get_page_table_entry(page_dir_entry_t *vm, uint32_t virtual_addr);
void free_page_tables(page_dir_entry_t *vm);
void kalloc_arch(void);

#endif
