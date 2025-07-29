#include <stdint.h>

// WASM start function - called instead of assembly boot code
void start(void) {
    // WASM doesn't need traditional boot setup
    // Memory and stack are already initialized by emscripten
    
    // Call the kernel entry point directly
    extern void _kernel_entry_c(void);
    _kernel_entry_c();
}