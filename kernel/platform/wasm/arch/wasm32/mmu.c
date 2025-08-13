#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <kernel/system.h>
#include <stdint.h>
#include <stddef.h>

// WASM uses a flat memory model - no hardware MMU
// These functions provide the MMU interface but don't actually do virtual memory translation

// WASM memory layout
#define WASM_KERNEL_BASE   0x80000000
#define WASM_USER_BASE     0x40000000
#define WASM_HEAP_BASE     0x20000000

uint32_t arch_get_kernel_phy_addr(void) {
    return WASM_KERNEL_BASE;
}

// Page directory functions
page_dir_entry_t* vm_new(void) {
    // Allocate a fake page directory
    page_dir_entry_t* vm = (page_dir_entry_t*)kalloc(PAGE_SIZE);
    if (vm == NULL)
        return NULL;
    
    // Clear the page directory
    uint32_t* vm_data = (uint32_t*)vm;
    for (int i = 0; i < PAGE_DIR_NUM; i++) {
        vm_data[i] = 0;
    }
    return vm;
}

void vm_free(page_dir_entry_t* vm) {
    if (vm != NULL) {
        kfree(vm);
    }
}

// Set virtual memory context (no-op for WASM)
void set_vm(page_dir_entry_t* vm) {
    (void)vm;
    // WASM doesn't have hardware MMU
}

// Map virtual to physical page
int32_t map_page(page_dir_entry_t* vm, 
                ewokos_addr_t vaddr, 
                ewokos_addr_t paddr, 
                uint32_t access,
                uint32_t pte_attr) {
    (void)vm;
    (void)vaddr;
    (void)paddr; 
    (void)access;
    (void)pte_attr;
    // WASM uses identity mapping, so mapping is always successful
    return 0;
}

// Unmap virtual page
void unmap_page(page_dir_entry_t* vm, ewokos_addr_t vaddr) {
    (void)vm;
    (void)vaddr;
    // WASM uses identity mapping, so unmapping is a no-op
}

ewokos_addr_t resolve_phy_address(page_dir_entry_t *vm, ewokos_addr_t virtual) {
    (void)vm;
    // Identity mapping in WASM
    return virtual;
}

// Get physical address for virtual address
ewokos_addr_t get_paddr(page_dir_entry_t* vm, ewokos_addr_t vaddr) {
    (void)vm;
    // Identity mapping in WASM
    return vaddr;
}

// Check if virtual address is mapped
int is_mapped(page_dir_entry_t* vm, ewokos_addr_t vaddr) {
    (void)vm;
    (void)vaddr;
    // Everything is mapped in WASM flat memory model
    return 1;
}

// Copy page directory
void vm_copy(page_dir_entry_t* dest_vm, page_dir_entry_t* src_vm) {
    (void)dest_vm;
    (void)src_vm;
    // No copying needed for WASM
}

// Flush TLB (no-op for WASM)
void flush_tlb(void) {
    // WASM doesn't have TLB
}

// Memory permission functions
uint32_t get_access_flags(page_dir_entry_t* vm, ewokos_addr_t vaddr) {
    (void)vm;
    (void)vaddr;
    // Return full access for WASM
    return AP_RW_RW;
}

void set_access_flags(page_dir_entry_t* vm, ewokos_addr_t vaddr, uint32_t access) {
    (void)vm;
    (void)vaddr;
    (void)access;
    // No access control in WASM
}

// Platform-specific MMU initialization
void arch_vm_init(void) {
    // No hardware MMU to initialize
}

void arch_kernel_vm_init(void) {
    // No kernel VM setup needed
}

// Set translation table base (required by kernel)
void set_translation_table_base(ewokos_addr_t base) {
    (void)base;
    // WASM doesn't have MMU hardware
}