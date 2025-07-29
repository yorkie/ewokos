#include <stdint.h>
#include <stddef.h>
#include <kstring.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <kprintf.h>

// Forward declaration
void wasm_init_main(void);

// WASM-specific init process loader
int32_t load_init_proc(void) {
    // In WASM, we don't load from SD card, so create a simple init process
    // that just runs in kernel space for now
    
    proc_t* proc = proc_create(TASK_TYPE_PROC, NULL);
    if(proc == NULL) {
        return -1;
    }
    
    // Set up basic process info
    strcpy(proc->info.cmd, "init");
    proc->info.uid = -1;   // root
    
    // For WASM, we'll just set up a minimal process
    // In a real implementation, we would load the init binary
    // but for now we'll just return success to let the kernel continue
    
    return 0;
}

// Simple init main function for WASM - not used in this simplified version
void wasm_init_main(void) {
    extern int32_t kprintf(const char *format, ...);
    extern void yield(void);
    
    kprintf("ewokos init process started in WASM\n");
    
    // Simple loop that yields
    while(1) {
        yield(); // Give other processes a chance to run
    }
}