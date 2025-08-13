#include <kernel/irq.h>
#include <kernel/system.h>
#include <kernel/context.h>
#include <kstring.h>
#include <stdint.h>

// WASM doesn't have hardware interrupts
// These functions provide the interrupt interface but are mostly no-ops

// Interrupt state
static uint32_t wasm_irq_disabled = 1; // Start with interrupts disabled

void arch_enable_irq(uint32_t irq) {
    (void)irq;
    // WASM doesn't have hardware IRQs
}

void arch_disable_irq(uint32_t irq) {
    (void)irq;
    // WASM doesn't have hardware IRQs
}

void arch_irq_init(void) {
    // No hardware IRQ controller to initialize
}

uint32_t arch_irq_get_status(void) {
    // No pending interrupts in WASM
    return 0;
}

void arch_irq_clear(uint32_t irq) {
    (void)irq;
    // No IRQs to clear
}

// Software interrupt simulation for WASM
void wasm_trigger_timer_irq(void) {
    // This could be called by the WASM host to simulate timer interrupts
    if (!wasm_irq_disabled) {
        // Simulate timer interrupt processing
        // Since we can't call irq_do_timer0 directly, skip for now
    }
}

void wasm_trigger_uart_irq(void) {
    // This could be called by the WASM host to simulate UART interrupts  
    if (!wasm_irq_disabled) {
        // Simulate UART interrupt processing
        // Since we can't call irq_do_raw directly, skip for now
    }
}

// Global interrupt enable/disable
void __irq_enable(void) {
    wasm_irq_disabled = 0;
}

void __irq_disable(void) {
    wasm_irq_disabled = 1;
}

void _irq_enable(void) {
    wasm_irq_disabled = 0;
}

void _irq_disable(void) {
    wasm_irq_disabled = 1;
}

uint32_t __irq_disable_switch(void) {
    uint32_t old_state = wasm_irq_disabled;
    wasm_irq_disabled = 1;
    return old_state;
}

void __irq_enable_switch(uint32_t cpsr) {
    wasm_irq_disabled = cpsr;
}

// Check if interrupts are enabled
int arch_irq_enabled(void) {
    return !wasm_irq_disabled;
}

// Additional IRQ functions needed by the kernel
void irq_enable(uint32_t irq) {
    (void)irq;
    // WASM simulates IRQs through host callbacks
}

void irq_enable_cpsr(context_t* ctx) {
    (void)ctx;
    // WASM doesn't have CPSR, just enable globally
    wasm_irq_disabled = 0;
}