#include "wasm_types.h"

// Color utilities implementation
uint32_t argb(uint32_t a, uint32_t r, uint32_t g, uint32_t b) {
    return (a << 24) | (r << 16) | (g << 8) | b;
}

uint8_t color_a(uint32_t c) {
    return (c >> 24) & 0xFF;
}

uint8_t color_r(uint32_t c) {
    return (c >> 16) & 0xFF;
}

uint8_t color_g(uint32_t c) {
    return (c >> 8) & 0xFF;
}

uint8_t color_b(uint32_t c) {
    return c & 0xFF;
}