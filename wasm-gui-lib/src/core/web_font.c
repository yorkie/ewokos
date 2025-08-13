#include "web_font.h"
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>

static font_t* fonts[16] = {0}; // Simple font cache

EM_JS(void, js_measure_text, (const char* text, const char* font_name, int size, int* width, int* height), {
    const canvas = document.createElement('canvas');
    const ctx = canvas.getContext('2d');
    ctx.font = `${size}px ${UTF8ToString(font_name)}`;
    const metrics = ctx.measureText(UTF8ToString(text));
    
    setValue(width, metrics.width, 'i32');
    setValue(height, size, 'i32'); // Approximate height as font size
});

int font_init(void) {
    memset(fonts, 0, sizeof(fonts));
    return 0;
}

font_t* font_new(const char* fname, bool safe) {
    if (!fname) fname = "Arial";
    
    // Check if font already exists
    for (int i = 0; i < 16; i++) {
        if (fonts[i] && strcmp(fonts[i]->name, fname) == 0) {
            return fonts[i];
        }
    }
    
    // Create new font
    font_t* font = (font_t*)malloc(sizeof(font_t));
    if (!font) return NULL;
    
    memset(font, 0, sizeof(font_t));
    strncpy(font->name, fname, sizeof(font->name) - 1);
    font->id = 1; // Simple ID
    
    // Add to cache
    for (int i = 0; i < 16; i++) {
        if (!fonts[i]) {
            fonts[i] = font;
            break;
        }
    }
    
    return font;
}

int font_load(const char* name, const char* fname) {
    // In web environment, fonts are loaded via CSS
    // This is a no-op but maintains API compatibility
    return 0;
}

int font_free(font_t* font) {
    if (!font) return -1;
    
    // Remove from cache
    for (int i = 0; i < 16; i++) {
        if (fonts[i] == font) {
            fonts[i] = NULL;
            break;
        }
    }
    
    free(font);
    return 0;
}

void font_text_size(const char* text, font_t* font, int size, uint32_t* w, uint32_t* h) {
    if (!text || !w || !h) return;
    
    const char* font_name = font ? font->name : "Arial";
    int width = 0, height = 0;
    
    js_measure_text(text, font_name, size, &width, &height);
    
    *w = width;
    *h = height;
}