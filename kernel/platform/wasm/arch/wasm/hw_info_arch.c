#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include <kstring.h>

uint32_t _core_base_offset = 0;

void sys_info_init_arch(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));

	_sys_info.total_phy_mem_size = 128 * MB;
	_sys_info.total_usable_mem_size = _sys_info.total_phy_mem_size;
	_sys_info.mmio.size = 0;
	_sys_info.mmio.phy_base = 0;
	_sys_info.mmio.v_base = 0;

	_sys_info.phy_offset = 0;
	_sys_info.vector_base = 0;
	_sys_info.cores = 1;

	strcpy(_sys_info.machine, "wasm");
	strcpy(_sys_info.arch, "wasm");

	_sys_info.allocable_phy_mem_top = _sys_info.total_phy_mem_size;
}

void arch_vm(page_dir_entry_t* vm) {
	(void)vm;
}

int32_t check_mem_map_arch(ewokos_addr_t phy_base, uint32_t size) {
	(void)phy_base;
	(void)size;
	return 0;
}

int32_t mem_map_is_normal_ram_arch(ewokos_addr_t phy_base, uint32_t size) {
	ewokos_addr_t end = phy_base + size;
	return end >= phy_base && phy_base >= _sys_info.allocable_phy_mem_base &&
		end <= _sys_info.allocable_phy_mem_top;
}

int32_t arch_clone_proc_vm(page_dir_entry_t* vm, page_dir_entry_t* kernel_vm) {
	(void)vm;
	(void)kernel_vm;
	return 0;
}
