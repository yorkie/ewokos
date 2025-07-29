#include <stdint.h>
#include <kstring.h>
#include <kernel/proc.h>
#include <kernel/system.h>

// Forward declaration
void wasm_init_main(void);

// WASM-specific init process loader
int32_t load_init_proc(void) {
    // In WASM, we don't load from SD card, so create a simple init process
    // that just runs in kernel space for now
    
    proc_t* proc = proc_create(PROC_TYPE_PROC);
    if(proc == NULL) {
        return -1;
    }
    
    // Set up basic process info
    strncpy(proc->info.name, "init", PROC_NAME_MAX-1);
    proc->info.owner = 0; // root
    proc->info.pid = 1;   // PID 1
    
    // In a real implementation, we would load the init binary
    // For WASM, we'll just create a minimal process that yields
    proc->info.entry = (uint32_t)wasm_init_main;
    
    // Make it the current process
    set_current_proc(proc);
    
    return 0;
}

// Simple init main function for WASM
void wasm_init_main(void) {
    extern int32_t kprintf(const char *format, ...);
    extern void yield(void);
    
    kprintf("ewokos init process started in WASM\n");
    
    // Simple loop that yields
    while(1) {
        yield(); // Give other processes a chance to run
    }
}