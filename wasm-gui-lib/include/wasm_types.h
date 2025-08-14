#ifndef WASM_TYPES_H
#define WASM_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Basic types compatible with original EwokOS types
typedef uint32_t ewokos_addr_t;

// Graphics types (from graph/graph.h)
typedef struct {
    int32_t w;    
    int32_t h;    
} gsize_t;

typedef struct {
    int32_t x;    
    int32_t y;    
} gpos_t;

typedef struct {
    int32_t x;    
    int32_t y;    
    int32_t w;    
    int32_t h;    
} grect_t;

typedef struct {
    uint32_t *buffer;
    int32_t w;
    int32_t h;
    grect_t clip;
    bool need_free;
} graph_t;

// Font types (simplified)
typedef struct {
    char name[64];
    int32_t id;
    void* cache; // Will be implemented as web font
} font_t;

// Color utilities
uint32_t argb(uint32_t a, uint32_t r, uint32_t g, uint32_t b);
uint8_t  color_a(uint32_t c);
uint8_t  color_r(uint32_t c);
uint8_t  color_g(uint32_t c);
uint8_t  color_b(uint32_t c);

#ifdef __cplusplus
}
#endif

#endif // WASM_TYPES_H