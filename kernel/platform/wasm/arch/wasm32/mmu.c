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

// Simple kalloc implementation for WASM
static char wasm_heap[1024*1024]; // 1MB heap
static uint32_t heap_pos = 0;

static uint32_t wasm_kalloc(uint32_t size) {
    if (heap_pos + size >= sizeof(wasm_heap)) {
        return 0; // Out of memory
    }
    uint32_t addr = (uint32_t)&wasm_heap[heap_pos];
    heap_pos += size;
    return addr;
}

static void wasm_kfree(void* ptr) {
    (void)ptr;
    // Simple allocator doesn't support free
}

uint32_t arch_get_kernel_phy_addr(void) {
    return WASM_KERNEL_BASE;
}

// Page directory functions
page_dir_entry_t* vm_new(void) {
    // Allocate a fake page directory
    page_dir_entry_t* vm = (page_dir_entry_t*)wasm_kalloc(PAGE_SIZE);
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
        wasm_kfree(vm);
    }
}

// Copy page directory
void vm_copy(page_dir_entry_t* dest_vm, page_dir_entry_t* src_vm) {
    (void)dest_vm;
    (void)src_vm;
    // No copying needed for WASM
}

void set_access_flags(page_dir_entry_t* vm, ewokos_addr_t vaddr, uint32_t access) {
    (void)vm;
    (void)vaddr;
    (void)access;
    // No access control in WASM
}

// Global MMU functions needed by kernel
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