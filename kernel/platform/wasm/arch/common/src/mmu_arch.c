#include <mm/mmu_arch.h>
#include <mm/kalloc.h>
#include <kernel/system.h>
#include <stdint.h>
#include <stddef.h>

// WASM MMU implementation - simplified for flat memory model

void set_pte_flags(page_table_entry_t* pte, uint32_t pte_attr) {
    (void)pte;
    (void)pte_attr;
    // WASM doesn't need PTE flags
}

page_table_entry_t* get_page_table_entry(page_dir_entry_t *vm, ewokos_addr_t virtual) {
    (void)vm;
    (void)virtual;
    // Return NULL since WASM doesn't have real page tables
    return NULL;
}

void free_page_tables(page_dir_entry_t *vm) {
    (void)vm;
    // No page tables to free in WASM
}

void __set_translation_table_base(ewokos_addr_t base) {
    (void)base;
    // WASM doesn't have MMU hardware
}

void __flush_tlb(void) {
    // WASM doesn't have TLB
}