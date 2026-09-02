#include <kernel/context.h>
#include <stdint.h>

static uint32_t current_translation_table;
static uint32_t irq_enabled;

void __switch_to(context_t* prev, context_t* next) {
    (void)prev;
    (void)next;
}

void __irq_enable(void) {
    irq_enabled = 1;
}

void __irq_disable(void) {
    irq_enabled = 0;
}

uint32_t __int_off(void) {
    uint32_t previous = irq_enabled;
    irq_enabled = 0;
    return previous;
}

void __int_on(uint32_t previous) {
    irq_enabled = previous != 0;
}

void __flush_tlb(void) {
}

void __set_translation_table_base(uint32_t val) {
    current_translation_table = val;
}

int __core_id(void) {
    return 0;
}

void __flush_dcache_all(void) {
}

void __invalidate_dcache_all(void) {
}

void __invalidate_icache_all(void) {
}

void __set_vector_table(uint32_t val) {
    (void)val;
}

void *__memcpy32(void *target, const void *source, uint32_t n) {
    uint32_t *dst = (uint32_t*)target;
    const uint32_t *src = (const uint32_t*)source;
    uint32_t words = n / sizeof(uint32_t);
    uint32_t tail = n % sizeof(uint32_t);

    while(words-- > 0)
        *dst++ = *src++;
    if(tail > 0) {
        uint8_t *dst8 = (uint8_t*)dst;
        const uint8_t *src8 = (const uint8_t*)src;
        while(tail-- > 0)
            *dst8++ = *src8++;
    }
    return target;
}
