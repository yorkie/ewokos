#include <stdint.h>
#include <emscripten.h>
#include <ewokos_config.h>

// WASM system functions - simplified replacements for ARM-specific functions

uint32_t __attribute__((weak)) get_core_id(void) {
    return 0; // Single core in WASM
}

void __attribute__((weak)) halt(void) {
    // In WASM, we can't really halt, so just infinite loop
    while(1) {
        emscripten_sleep(1000);
    }
}

void __attribute__((weak)) _delay_usec(uint32_t usec) {
    // Convert microseconds to milliseconds for emscripten_sleep
    uint32_t msec = usec / 1000;
    if (msec == 0) msec = 1;
    emscripten_sleep(msec);
}

void __attribute__((weak)) _delay_msec(uint32_t msec) {
    emscripten_sleep(msec);
}

// Cache operations - no-ops in WASM
void __attribute__((weak)) flush_dcache(void) {
    // No-op in WASM
}

void __attribute__((weak)) flush_icache(void) {
    // No-op in WASM
}

void __attribute__((weak)) flush_tlb(void) {
    // No-op in WASM
}

// Context switching - simplified for WASM
void __attribute__((weak)) switch_to_user_mode(void) {
    // No-op in WASM - no user/kernel mode distinction
}

// System control functions
void __attribute__((weak)) system_reset(void) {
    // In WASM, we can't reset the system, so just reload the page
    EM_ASM({
        location.reload();
    });
}

// Additional system functions required by the kernel
void __attribute__((weak)) set_vector_table(ewokos_addr_t addr) {
    // No-op in WASM
    (void)addr;
}

void __attribute__((weak)) set_translation_table_base(ewokos_addr_t base) {
    // No-op in WASM  
    (void)base;
}