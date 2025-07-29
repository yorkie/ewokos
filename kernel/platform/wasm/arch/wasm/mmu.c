#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <emscripten.h>
#include <ewokos_config.h>
#include <mm/mmu_arch.h>

// WASM MMU functions - simplified since WASM has its own memory management

// Virtual to physical address conversion - in WASM, they're the same
#define V2P(addr) (addr)
#define P2V(addr) (addr)

void __attribute__((weak)) set_translation_table_base(uint32_t base) {
    // No-op in WASM - memory management handled by browser
    (void)base;
}

void __attribute__((weak)) map_pages(page_dir_entry_t* vm, uint32_t vaddr, uint32_t paddr_start, uint32_t paddr_end, uint32_t access, uint32_t attr) {
    // No-op in WASM - memory is linear and managed by browser
    (void)vm; (void)vaddr; (void)paddr_start; (void)paddr_end; (void)access; (void)attr;
}

void __attribute__((weak)) map_pages_size(page_dir_entry_t* vm, uint32_t vaddr, uint32_t paddr, uint32_t size, uint32_t access, uint32_t attr) {
    // No-op in WASM
    (void)vm; (void)vaddr; (void)paddr; (void)size; (void)access; (void)attr;
}

void __attribute__((weak)) map_allocable_pages(page_dir_entry_t* vm) {
    // No-op in WASM
    (void)vm;
}

uint32_t __attribute__((weak)) kalloc_append(uint32_t start, uint32_t end) {
    // In WASM, we use malloc/free from emscripten
    return end - start;
}

void __attribute__((weak)) kalloc_arch(void) {
    // No-op in WASM
}

// Memory allocation functions that use emscripten's malloc
void* __attribute__((weak)) kmalloc(uint32_t size) {
    return malloc(size);
}

void __attribute__((weak)) kfree(void* ptr) {
    free(ptr);
}

// Page allocation using regular malloc in WASM
void* __attribute__((weak)) kmalloc_4k(void) {
    return malloc(PAGE_SIZE);
}

void __attribute__((weak)) kfree4k(void* ptr) {
    free(ptr);
}

// Additional MMU functions needed by the kernel
int32_t __attribute__((weak)) map_page(page_dir_entry_t *vm, 
    ewokos_addr_t virtual_addr, 
    ewokos_addr_t physical,
    uint32_t access_permissions, 
    uint32_t pte_attr) {
    // No-op in WASM
    (void)vm; (void)virtual_addr; (void)physical; (void)access_permissions; (void)pte_attr;
    return 0;
}

void __attribute__((weak)) unmap_page(page_dir_entry_t *vm, ewokos_addr_t virtual_addr) {
    // No-op in WASM
    (void)vm; (void)virtual_addr;
}

ewokos_addr_t __attribute__((weak)) resolve_phy_address(page_dir_entry_t *vm, ewokos_addr_t virtual) {
    // In WASM, virtual == physical
    (void)vm;
    return virtual;
}

page_table_entry_t* __attribute__((weak)) get_page_table_entry(page_dir_entry_t *vm, ewokos_addr_t virtual) {
    // Return NULL in WASM
    (void)vm; (void)virtual;
    return NULL;
}

void __attribute__((weak)) free_page_tables(page_dir_entry_t *vm) {
    // No-op in WASM
    (void)vm;
}

void __attribute__((weak)) __set_translation_table_base(ewokos_addr_t base) {
    // No-op in WASM
    (void)base;
}

void __attribute__((weak)) __flush_tlb(void) {
    // No-op in WASM
}