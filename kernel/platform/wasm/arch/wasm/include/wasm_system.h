#ifndef WASM_SYSTEM_H
#define WASM_SYSTEM_H

#include <stdint.h>
#include <ewokos_config.h>

// WASM-specific system function declarations
void set_vector_table(ewokos_addr_t addr);
void set_translation_table_base(ewokos_addr_t base);
void flush_tlb(void);
void flush_dcache(void);

#endif