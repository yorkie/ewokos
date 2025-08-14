#include <font/font.h>
#include <wasm_ewok_compat.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// WebAssembly font implementation using browser fonts

typedef struct {
    char name[64];
    int size;
    bool fixed;
} font_wasm_t;

int font_init(void) {
    return 0; // WebAssembly doesn't need special font initialization
}

const char* font_name_by_fname(const char* fname) {
    static char ret[128];
    memset(ret, 0, 128);
    
    if (!fname) return "Arial";
    
    // Extract name from path
    const char* base = get_relative_name(fname);
    strncpy(ret, base, 127);
    
    // Remove extension
    char* dot = strrchr(ret, '.');
    if (dot) *dot = '\0';
    
    return ret;
}

int font_load(const char* name, const char* fname) {
    // WebAssembly stub - fonts are handled by browser
    return 0;
}

font_t* font_new(const char* name, bool fixed) {
    font_wasm_t* font = (font_wasm_t*)malloc(sizeof(font_wasm_t));
    if (!font) return NULL;
    
    if (name) {
        strncpy(font->name, name, 63);
        font->name[63] = '\0';
    } else {
        strcpy(font->name, "Arial");
    }
    
    font->size = 14;
    font->fixed = fixed;
    
    return (font_t*)font;
}

void font_free(font_t* font) {
    if (font) {
        free(font);
    }
}

// JavaScript font measurement functions
#ifdef __EMSCRIPTEN__
EM_JS(void, js_measure_text, (const char* text, const char* font_name, int font_size, int* width, int* height), {
    const canvas = document.createElement('canvas');
    const ctx = canvas.getContext('2d');
    ctx.font = font_size + 'px ' + UTF8ToString(font_name);
    
    const metrics = ctx.measureText(UTF8ToString(text));
    setValue(width, metrics.width, 'i32');
    setValue(height, font_size, 'i32'); // Approximate height
});

EM_JS(void, js_draw_text, (const char* text, int x, int y, const char* font_name, int font_size, uint32_t color, uint32_t* buffer, int buf_w, int buf_h), {
    // This would be implemented to draw text to the buffer
    // For now, just a stub
});
#else
// Stub implementations for non-Emscripten builds
static void js_measure_text(const char* text, const char* font_name, int font_size, int* width, int* height) {
    if (width) *width = strlen(text) * 8; // Rough estimate
    if (height) *height = font_size;
}
static void js_draw_text(const char* text, int x, int y, const char* font_name, int font_size, uint32_t color, uint32_t* buffer, int buf_w, int buf_h) {}
#endif

gsize_t font_text_size(font_t* font, const char* str, uint32_t len, int size) {
    gsize_t ret = {0, 0};
    
    if (!font || !str) return ret;
    
    font_wasm_t* f = (font_wasm_t*)font;
    int font_size = size > 0 ? size : f->size;
    
    // Measure text using JavaScript
    int width, height;
    js_measure_text(str, f->name, font_size, &width, &height);
    
    ret.w = width;
    ret.h = height;
    
    return ret;
}

const char* font_get_name(font_t* font) {
    if (!font) return "Arial";
    
    font_wasm_t* f = (font_wasm_t*)font;
    return f->name;
}

bool font_fixed(font_t* font) {
    if (!font) return false;
    
    font_wasm_t* f = (font_wasm_t*)font;
    return f->fixed;
}

void graph_draw_text_font(graph_t* g, int32_t x, int32_t y, const char* str,
        font_t* font, uint32_t size, uint32_t color) {
    if (!g || !str || !font || !g->buffer) return;
    
    font_wasm_t* f = (font_wasm_t*)font;
    int font_size = size > 0 ? size : f->size;
    
    // Simple bitmap text rendering for demo purposes
    // This is a minimal implementation that draws rectangles for characters
    int char_width = font_size / 2;
    int char_height = font_size;
    
    for (int i = 0; str[i] && x + i * char_width < g->w; i++) {
        if (y >= 0 && y + char_height <= g->h) {
            // Draw a simple rectangle for each character as placeholder
            for (int cy = 0; cy < char_height; cy++) {
                for (int cx = 0; cx < char_width; cx++) {
                    int px = x + i * char_width + cx;
                    int py = y + cy;
                    if (px >= 0 && px < g->w && py >= 0 && py < g->h) {
                        // Simple character pattern (just outline for demo)
                        if (cx == 0 || cy == 0 || cx == char_width-1 || cy == char_height-1) {
                            g->buffer[py * g->w + px] = color;
                        }
                    }
                }
            }
        }
    }
}

void graph_draw_text_font_align(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h,
        const char* str, font_t* font, uint32_t size, uint32_t color, uint32_t align) {
    if (!g || !str || !font) return;
    
    // Simple left-aligned implementation for now
    graph_draw_text_font(g, x, y, str, font, size, color);
}

void graph_draw_text(graph_t* g, int32_t x, int32_t y, const char* str,
        font_t* font, uint32_t size, uint32_t color) {
    graph_draw_text_font(g, x, y, str, font, size, color);
}

#ifdef __cplusplus
}
#endif