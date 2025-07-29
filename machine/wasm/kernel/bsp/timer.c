#include <dev/timer.h>
#include <stdint.h>
#include <emscripten.h>

static uint32_t timer_freq = 100; // 100Hz default

void timer_set_interval(uint32_t id, uint32_t times_per_sec) {
    (void)id; // WASM only supports one timer
    timer_freq = times_per_sec;
    // The actual timer is managed by JavaScript in pre.js
}

uint32_t timer_get_ticks(uint32_t id) {
    (void)id;
    // Return JavaScript timestamp in milliseconds converted to ticks
    return (uint32_t)emscripten_get_now();
}

void timer_clear_interrupt(uint32_t id) {
    (void)id;
    // No-op in WASM - interrupt handling is different
}

// Timer interrupt handler - called from JavaScript
void timer_interrupt_handler(void) {
    extern void schedule(void);
    // Call the kernel scheduler
    schedule();
}