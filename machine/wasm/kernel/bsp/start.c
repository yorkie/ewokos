#include <kernel/system.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>

// WASM doesn't need a complex boot sequence like ARM platforms
// The _start function in system.c calls _kernel_entry_c directly

// Boot initialization for WASM platform
void _boot_start(void) {
    // WASM doesn't need MMU setup since it uses flat memory model
    // This function exists for compatibility but does nothing
}