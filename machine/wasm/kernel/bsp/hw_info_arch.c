#include <kernel/hw_info.h>
#include <kernel/system.h>

// WASM hardware information
void get_hw_info(sys_info_t* info) {
    if (info == NULL)
        return;

    // WASM memory layout - simplified flat model
    info->phy_offset = 0;
    info->total_phy_mem_size = 64*MB;  // 64MB total memory
    info->total_usable_mem_size = 60*MB;  // 60MB usable
    info->kmalloc_size = 8*MB;  // 8MB for kernel heap
    
    // MMIO region (simulated)
    info->mmio.phy_base = 0x10000000;
    info->mmio.v_base = 0x10000000;
    info->mmio.size = 1*MB;
    
    // DMA region (simulated)  
    info->sys_dma.phy_base = 0x11000000;
    info->sys_dma.v_base = 0x11000000;
    info->sys_dma.size = 1*MB;
    
    // GPU memory (not used in WASM)
    info->gpu.v_base = 0;
    info->gpu.max_size = 0;
}