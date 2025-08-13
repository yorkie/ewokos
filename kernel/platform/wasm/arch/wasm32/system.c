#include <kernel/kernel.h>
#include <kernel/system.h>
#include <kernel/context.h>
#include <stdint.h>

// Host simulation of WASM runtime
#ifdef HOST_SIMULATION

#define _GNU_SOURCE
#include <sys/time.h>
#include <unistd.h>

// WASM platform global variables that need to be defined
ewokos_addr_t _allocable_phy_mem_base = 0x20000000;  // 512MB base
ewokos_addr_t _allocable_phy_mem_top = 0x40000000;   // 1GB top

// WASM linker symbols (simulated)
char _kernel_end[1];
char _bss_start[1]; 
char _bss_end[1];

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

// Platform-specific early debug output
void _debug_output(const char* s) {
    if (s) {
        uint32_t len = 0;
        const char* p = s;
        while (*p++) len++;  // Simple strlen
        wasm_debug_print(s, len);
    }
}

// WASM timer function
void timer_set_interval(uint32_t id, uint32_t times_per_sec) {
    (void)id;
    (void)times_per_sec;
    // Timer is handled by the host environment
}

// WASM VM function 
void arch_vm(page_dir_entry_t* vm) {
    (void)vm;
    // WASM doesn't need VM setup
}

// WASM device initialization functions
void uart_dev_init(int baud) {
    (void)baud;
    // UART is always ready in WASM simulation
}

void sd_init(void) {
    // SD is simulated by the host
}

// WASM memory allocation architecture function
void kalloc_arch(void) {
    // No special architecture setup needed for WASM
}

// WASM cache and system functions (no-ops)
void __flush_dcache_all(void) {
    // WASM doesn't have caches
}

void __set_vector_table(ewokos_addr_t base) {
    (void)base;
    // WASM doesn't have interrupt vectors
}

// WASM system info initialization
void sys_info_init_arch(void) {
    // WASM-specific system info already set in hw_info_arch.c
}

// WASM interrupt table symbols (simulated)
uint32_t interrupt_table_start = 0;
uint32_t interrupt_table_end = 0;

// WASM timer function
uint64_t timer_read_sys_usec(void) {
    return (uint64_t)wasm_get_time_ms() * 1000; // Convert ms to usec
}

// WASM missing functions needed for kernel
void uart_write(char c) {
    wasm_debug_print(&c, 1);
}

int sd_dev_read(uint32_t block_addr) {
    (void)block_addr;
    return 0; // Simulate success
}

void sd_dev_read_done(void) {
    // No-op for WASM
}

uint32_t irq_get(void) {
    return 0; // No pending IRQs
}

void timer_clear_interrupt(void) {
    // No-op for WASM
}

void dump_ctx(context_t* ctx) {
    (void)ctx;
    // No-op for WASM - could implement debug output here
}

void irq_arch_init(void) {
    // No-op for WASM
}

void irq_disable_cpsr(context_t* ctx) {
    (void)ctx;
    // No-op for WASM
}

int check_mem_map_arch(ewokos_addr_t phy_base, uint32_t size) {
    (void)phy_base;
    (void)size;
    return 0; // Allow all memory access in WASM
}

void __memcpy32(void* dest, const void* src, uint32_t sz) {
    // Simple memcpy implementation
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (uint32_t i = 0; i < sz; i++) {
        d[i] = s[i];
    }
}

// Additional system functions needed by kernel
void flush_dcache(void) {
    // WASM doesn't have caches
}

void flush_tlb(void) {
    // WASM doesn't have TLB
}

void set_translation_table_base(ewokos_addr_t base) {
    (void)base;
    // WASM doesn't have MMU hardware
}

void set_vector_table(ewokos_addr_t base) {
    (void)base;
    // WASM doesn't have interrupt vectors
}

void _delay_msec(uint32_t ms) {
    // Simple delay using time
    uint64_t start = timer_read_sys_usec();
    uint64_t target = start + (ms * 1000);
    while (timer_read_sys_usec() < target) {
        // Busy wait
    }
}

// Kernel lock functions for SMP (no-op for WASM)
// These are defined as macros in system.h for WASM