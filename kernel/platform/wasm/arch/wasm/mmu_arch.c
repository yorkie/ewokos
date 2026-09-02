#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <kernel/hw_info.h>
#include <kstring.h>
#include <stddef.h>
#include <stdint.h>

static page_table_entry_t *get_table(page_dir_entry_t *vm, uint32_t virtual_addr, int create) {
    uint32_t l1 = virtual_addr >> 22;
    page_table_entry_t *table;

    if((vm[l1].value & WASM_PTE_PRESENT) == 0) {
        if(!create)
            return NULL;
        table = (page_table_entry_t*)kalloc_page();
        if(table == NULL)
            return NULL;
        memset(table, 0, PAGE_TABLE_SIZE);
        vm[l1].value = (V2P(table) & WASM_PTE_ADDR_MASK) |
            WASM_PTE_PRESENT | WASM_PTE_WRITE | WASM_PTE_USER;
    }
    return (page_table_entry_t*)P2V(vm[l1].value & WASM_PTE_ADDR_MASK);
}

int32_t map_page(page_dir_entry_t *vm, uint32_t virtual_addr,
		     uint32_t physical, uint32_t permissions, uint32_t pte_attr) {
    page_table_entry_t *table = get_table(vm, virtual_addr, 1);
    uint32_t l2 = (virtual_addr >> 12) & 0x3ffu;
    uint32_t flags = WASM_PTE_PRESENT;

    if(table == NULL)
        return -1;
    if(permissions != AP_RW_R)
        flags |= WASM_PTE_WRITE;
    if(permissions == AP_RW_RW || permissions == AP_RW_R)
        flags |= WASM_PTE_USER;
    (void)pte_attr;
    table[l2].value = (physical & WASM_PTE_ADDR_MASK) | flags;
    return 0;
}

void unmap_page(page_dir_entry_t *vm, uint32_t virtual_addr) {
    page_table_entry_t *table = get_table(vm, virtual_addr, 0);
    if(table != NULL)
        table[(virtual_addr >> 12) & 0x3ffu].value = 0;
}

uint32_t resolve_phy_address(page_dir_entry_t *vm, uint32_t virtual_addr) {
    page_table_entry_t *entry = get_page_table_entry(vm, virtual_addr);
    if(entry == NULL)
        return 0;
    return (entry->value & WASM_PTE_ADDR_MASK) | (virtual_addr & (PAGE_SIZE - 1));
}

page_table_entry_t* get_page_table_entry(page_dir_entry_t *vm, uint32_t virtual_addr) {
    page_table_entry_t *table = get_table(vm, virtual_addr, 0);
    page_table_entry_t *entry;

    if(table == NULL)
        return NULL;
    entry = &table[(virtual_addr >> 12) & 0x3ffu];
    return (entry->value & WASM_PTE_PRESENT) != 0 ? entry : NULL;
}

void free_page_tables(page_dir_entry_t *vm) {
    for(uint32_t i = 0; i < PAGE_DIR_NUM; i++) {
        if((vm[i].value & WASM_PTE_PRESENT) != 0) {
            kfree_page((void*)P2V(vm[i].value & WASM_PTE_ADDR_MASK));
            vm[i].value = 0;
        }
    }
}

void kalloc_arch(void) {
    kalloc_append(P2V(_sys_info.allocable_phy_mem_base),
        P2V(_sys_info.allocable_phy_mem_top));
}

int32_t wasm_mmu_self_test(void) {
    page_dir_entry_t *vm = (page_dir_entry_t*)kalloc_page();
    void *page = kalloc_page();
    const uint32_t virtual_addr = 0x01023000u;
    uint32_t physical;

    if(vm == NULL || page == NULL)
        return 1;
    memset(vm, 0, PAGE_DIR_SIZE);
    physical = V2P(page);
    if(map_page(vm, virtual_addr, physical, AP_RW_RW, PTE_ATTR_WRBACK) != 0)
        return 2;
    if(resolve_phy_address(vm, virtual_addr + 123u) != physical + 123u)
        return 3;
    unmap_page(vm, virtual_addr);
    if(resolve_phy_address(vm, virtual_addr) != 0)
        return 4;
    free_page_tables(vm);
    kfree_page(page);
    kfree_page(vm);
    return 0;
}
