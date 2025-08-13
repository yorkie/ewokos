#include <kernel/kernel.h>
#include <kernel/system.h>
#include <stdint.h>

// Host simulation of WASM runtime
#ifdef HOST_SIMULATION

#define _GNU_SOURCE
#include <sys/time.h>
#include <unistd.h>

static char uart_input_buffer[256] = {0};
static int uart_input_pos = 0;
static int uart_input_len = 0;
static uint64_t start_time_ms = 0;

// Simple time function for host simulation
static uint64_t get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

// WASM host functions for simulation
void wasm_debug_print(const char* str, uint32_t len) {
    // Use write to avoid stdio conflicts
    write(1, str, len);
}

uint32_t wasm_get_time_ms(void) {
    uint64_t current_ms = get_time_ms();
    if (start_time_ms == 0) {
        start_time_ms = current_ms;
    }
    return (uint32_t)(current_ms - start_time_ms);
}

void wasm_exit(int32_t code) {
    const char* msg = "\nEwokOS WASM simulation exited\n";
    write(1, msg, 30);
    _exit(code);
}

int wasm_uart_has_data(void) {
    // For simulation, we'll just check if we have buffered input
    return uart_input_pos < uart_input_len;
}

char wasm_uart_read_char(void) {
    if (uart_input_pos < uart_input_len) {
        return uart_input_buffer[uart_input_pos++];
    }
    return 0;
}

int wasm_sd_read_block(uint32_t block, void* buffer, uint32_t size) {
    (void)block;
    (void)buffer;
    (void)size;
    // No SD simulation for now
    return -1;
}

int wasm_sd_write_block(uint32_t block, const void* buffer, uint32_t size) {
    (void)block;
    (void)buffer;
    (void)size;
    // No SD simulation for now
    return -1;
}

uint32_t wasm_sd_get_size(void) {
    return 0; // No SD simulation
}

int wasm_load_init_process(const char* path, void** elf_data, int* size) {
    (void)path;
    (void)elf_data;
    (void)size;
    // No init process loading for basic simulation
    return -1;
}

// Main entry point for host simulation
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    const char* msg = "EwokOS WASM Host Simulation Starting...\n";
    write(1, msg, 40);
    
    // Call kernel entry point
    _kernel_entry_c();
    
    return 0;
}

#else // Real WASM environment

// WASM imports - functions provided by the host environment  
extern void wasm_debug_print(const char* str, uint32_t len);
extern uint32_t wasm_get_time_ms(void);
extern void wasm_exit(int32_t code);

// WASM system control functions
void __attribute__((export_name("_start"))) _start(void) {
    // WASM entry point - call kernel main directly
    _kernel_entry_c();
}

#endif

void halt(void) {
    wasm_exit(0);
    // Should not return
    while(1);
}

// WASM-specific halt implementation
inline void arch_halt(void) {
    halt();
}

void __irq_enable(void) {
    // WASM doesn't have hardware interrupts
}

void __irq_disable(void) {
    // WASM doesn't have hardware interrupts  
}

void _irq_enable(void) {
    // WASM doesn't have hardware interrupts
}

void _irq_disable(void) {
    // WASM doesn't have hardware interrupts
}

uint32_t __irq_disable_switch(void) {
    // WASM doesn't have hardware interrupts
    return 0;
}

void __irq_enable_switch(uint32_t cpsr) {
    (void)cpsr;
    // WASM doesn't have hardware interrupts
}

// System information for WASM
void arch_vm_init(void) {
    // WASM uses flat memory model - no MMU setup needed
}

void arch_kernel_vm_init(void) {
    // WASM uses flat memory model
}

uint32_t arch_get_core_id(void) {
    // Single core for WASM
    return 0;
}

void arch_halt_core(uint32_t core_id) {
    (void)core_id;
    halt();
}

void arch_stop_core(uint32_t core_id) {
    (void)core_id;
    halt();
}

void arch_start_core(uint32_t core_id, void* entry) {
    (void)core_id;
    (void)entry;
    // WASM is single-core, ignore
}

// Memory barrier functions (no-op for WASM)
void dmb(void) {
    // No memory barriers needed in WASM
}

void dsb(void) {
    // No memory barriers needed in WASM
}

void isb(void) {
    // No instruction barriers needed in WASM
}

// Cache functions (no-op for WASM)
void flush_dcache_all(void) {
    // WASM doesn't have caches
}

void flush_dcache_range(uint32_t start, uint32_t end) {
    (void)start;
    (void)end;
    // WASM doesn't have caches
}

void invalidate_dcache_range(uint32_t start, uint32_t end) {
    (void)start;
    (void)end;
    // WASM doesn't have caches
}

// Platform-specific early debug output
void _debug_output(const char* s) {
    if (s) {
        uint32_t len = 0;
        const char* p = s;
        while (*p++) len++;  // Simple strlen
        wasm_debug_print(s, len);
    }
}