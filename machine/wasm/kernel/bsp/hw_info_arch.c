#include <kernel/hw_info.h>
#include <kstring.h>
#include <stdint.h>

void hw_info_arch_init(void) {
    // Initialize WASM-specific hardware info
    strcpy(_sys_info.machine, "wasm");
    strcpy(_sys_info.arch, "wasm32");
    _sys_info.cores = 1;
    _sys_info.phy_offset = 0;
    _sys_info.total_phy_mem_size = 32 * 1024 * 1024; // 32MB
    _sys_info.total_usable_mem_size = 28 * 1024 * 1024; // 28MB usable
    _sys_info.kmalloc_size = 4 * 1024 * 1024; // 4MB for kernel malloc
    
    // WASM doesn't have traditional MMIO, but we set up virtual addresses
    _sys_info.mmio.phy_base = 0x10000000;
    _sys_info.mmio.v_base = 0x10000000;
    _sys_info.mmio.size = 1024 * 1024; // 1MB
    
    // DMA area
    _sys_info.sys_dma.phy_base = 0x20000000;
    _sys_info.sys_dma.v_base = 0x20000000;
    _sys_info.sys_dma.size = 64 * 1024; // 64KB
    
    // GPU framebuffer area
    _sys_info.gpu.v_base = 0x30000000;
    _sys_info.gpu.max_size = 8 * 1024 * 1024; // 8MB
    
    // Vector base for interrupts (not really used in WASM)
    _sys_info.vector_base = 0;
}