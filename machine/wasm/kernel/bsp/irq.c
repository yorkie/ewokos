#include <kernel/irq.h>
#include <kernel/system.h>
#include <dev/timer.h>

// WASM machine IRQ definitions
#define WASM_IRQ_TIMER 0
#define WASM_IRQ_UART0 1

// IRQ enable/disable for WASM platform
void irq_arch_enable(uint32_t irq) {
    (void)irq;
    // WASM simulates IRQs through host callbacks
}

void irq_arch_disable(uint32_t irq) {
    (void)irq;
    // WASM simulates IRQs through host callbacks
}

// Initialize IRQ system for WASM
void irq_arch_init(void) {
    // No hardware IRQ controller to initialize
    // IRQs will be triggered by the WASM host environment
}

// Get pending IRQ status
uint32_t irq_arch_get_status(void) {
    // No hardware status register
    return 0;
}

// Clear IRQ
void irq_arch_clear(uint32_t irq) {
    (void)irq;
    // No hardware IRQ to clear
}