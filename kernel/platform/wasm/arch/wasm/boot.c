#include <stdint.h>
#include <emscripten.h>

// Global variables for WASM environment
uint32_t _interrupt_table_start = 0;
uint32_t _interrupt_table_end = 0;
uint32_t _bss_start = 0;
uint32_t _bss_end = 0;

// WASM entry point - this will be called from JavaScript
int main() {
    // Call the kernel entry point
    extern void _kernel_entry_c(void);
    _kernel_entry_c();
    return 0;
}

// WASM doesn't have traditional boot assembly, so we provide stubs
void __attribute__((weak)) _boot_start(void) {
    // No-op for WASM - memory initialization handled by emscripten
}

// Export main function for JavaScript access
EMSCRIPTEN_KEEPALIVE
int emscripten_main() {
    return main();
}