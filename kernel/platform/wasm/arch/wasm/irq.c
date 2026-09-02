#include <kernel/irq.h>
#include <stdint.h>

void irq_init_arch(void) {
}

void irq_enable_arch(uint32_t irq) {
    (void)irq;
}

void irq_enable_core_arch(uint32_t core, uint32_t irq) {
    (void)core;
    (void)irq;
}

void irq_disable_arch(uint32_t irq) {
    (void)irq;
}

void irq_clear_arch(uint32_t irq) {
    (void)irq;
}

void irq_clear_core_arch(uint32_t core, uint32_t irq) {
    (void)core;
    (void)irq;
}

void irq_eoi_arch(uint32_t irq) {
    (void)irq;
}

uint32_t irq_get_arch(void) {
    return 0;
}

uint32_t irq_get_unified_arch(uint32_t irq) {
    return irq;
}

void dump_ctx(context_t *ctx) {
    (void)ctx;
}
