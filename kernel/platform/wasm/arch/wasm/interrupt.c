#include <stdint.h>
#include <emscripten.h>

// WASM interrupt handling - simplified since we don't have real interrupts
void __attribute__((weak)) __irq_enable(void) {
    // No-op in WASM
}

void __attribute__((weak)) __irq_disable(void) {
    // No-op in WASM  
}

void __attribute__((weak)) interrupt_table_start(void) {
    // No-op in WASM
}

void __attribute__((weak)) interrupt_table_end(void) {
    // No-op in WASM
}

// Timer interrupt simulation - called from JavaScript
EMSCRIPTEN_KEEPALIVE
void wasm_timer_interrupt() {
    // Call the kernel timer handler if it exists
    extern void timer_interrupt_handler(void);
    timer_interrupt_handler();
}

// WASM doesn't have traditional interrupt vectors
void set_interrupt_vector(uint32_t vector, void* handler) {
    // No-op in WASM
}