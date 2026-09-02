#include <dev/timer.h>
#include <stdint.h>

extern uint64_t wasm_host_now_usec(void);

void timer_set_interval(uint32_t id, uint32_t freq) {
    (void)id;
    (void)freq;
}

uint64_t timer_read_sys_usec(void) {
    return wasm_host_now_usec();
}

void timer_clear_interrupt(uint32_t id) {
    (void)id;
}
