#include <dev/timer.h>
#include <kernel/irq.h>
#include <kernel/system.h>

// WASM imports for time
extern uint32_t wasm_get_time_ms(void);

static uint32_t timer_start_time = 0;

// Initialize timer for WASM
void timer_arch_init(void) {
    timer_start_time = wasm_get_time_ms();
}

// Set timer interval (WASM doesn't have hardware timer)
void timer_arch_set_interval(uint32_t microsec) {
    (void)microsec;
    // WASM host will simulate timer interrupts
}

// Get current timer value
uint32_t timer_arch_get_ticks(void) {
    uint32_t current_time = wasm_get_time_ms();
    return (current_time - timer_start_time) * 1000; // Convert to microseconds
}

// Clear timer interrupt
void timer_arch_clear_irq(void) {
    // No hardware timer interrupt to clear
}

// Enable timer
void timer_arch_enable(void) {
    // Timer is always enabled in WASM
}

// Disable timer  
void timer_arch_disable(void) {
    // Cannot disable timer in WASM
}

// WASM-specific function to trigger timer interrupt from host
void wasm_timer_tick(void) {
    // This will be called by the WASM host to simulate timer interrupts
    timer_irq_handler();
}