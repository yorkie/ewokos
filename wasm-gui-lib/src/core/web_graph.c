#include "web_graph.h"
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>

// External JavaScript functions for Canvas API
EM_JS(void, js_canvas_set_pixel, (const char* canvas_id, int x, int y, int r, int g, int b, int a), {
    const canvas = document.getElementById(UTF8ToString(canvas_id));
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const imageData = ctx.createImageData(1, 1);
    imageData.data[0] = r;
    imageData.data[1] = g;
    imageData.data[2] = b;
    imageData.data[3] = a;
    ctx.putImageData(imageData, x, y);
});

EM_JS(void, js_canvas_fill_rect, (const char* canvas_id, int x, int y, int w, int h, int r, int g, int b, int a), {
    const canvas = document.getElementById(UTF8ToString(canvas_id));
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    ctx.fillStyle = `rgba(${r},${g},${b},${a/255})`;
    ctx.fillRect(x, y, w, h);
});

EM_JS(void, js_canvas_draw_line, (const char* canvas_id, int x1, int y1, int x2, int y2, int r, int g, int b, int a), {
    const canvas = document.getElementById(UTF8ToString(canvas_id));
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    ctx.strokeStyle = `rgba(${r},${g},${b},${a/255})`;
    ctx.beginPath();
    ctx.moveTo(x1, y1);
    ctx.lineTo(x2, y2);
    ctx.stroke();
});

EM_JS(void, js_canvas_draw_circle, (const char* canvas_id, int x, int y, int radius, int r, int g, int b, int a, int fill), {
    const canvas = document.getElementById(UTF8ToString(canvas_id));
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const style = `rgba(${r},${g},${b},${a/255})`;
    ctx.beginPath();
    ctx.arc(x, y, radius, 0, 2 * Math.PI);
    if (fill) {
        ctx.fillStyle = style;
        ctx.fill();
    } else {
        ctx.strokeStyle = style;
        ctx.stroke();
    }
});

EM_JS(void, js_canvas_clear, (const char* canvas_id, int r, int g, int b, int a), {
    const canvas = document.getElementById(UTF8ToString(canvas_id));
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    ctx.fillStyle = `rgba(${r},${g},${b},${a/255})`;
    ctx.fillRect(0, 0, canvas.width, canvas.height);
});

EM_JS(void, js_canvas_draw_text, (const char* canvas_id, int x, int y, const char* text, const char* font, int size, int r, int g, int b, int a), {
    const canvas = document.getElementById(UTF8ToString(canvas_id));
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    ctx.font = `${size}px ${UTF8ToString(font)}`;
    ctx.fillStyle = `rgba(${r},${g},${b},${a/255})`;
    ctx.fillText(UTF8ToString(text), x, y);
});

EM_JS(void, js_canvas_flush_buffer, (const char* canvas_id, uint32_t* buffer, int w, int h), {
    const canvas = document.getElementById(UTF8ToString(canvas_id));
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const imageData = ctx.createImageData(w, h);
    
    const buf = new Uint32Array(Module.HEAPU8.buffer, buffer, w * h);
    for (let i = 0; i < buf.length; i++) {
        const pixel = buf[i];
        const idx = i * 4;
        imageData.data[idx + 0] = (pixel >> 16) & 0xFF; // R
        imageData.data[idx + 1] = (pixel >> 8) & 0xFF;  // G
        imageData.data[idx + 2] = pixel & 0xFF;         // B
        imageData.data[idx + 3] = (pixel >> 24) & 0xFF; // A
    }
    
    ctx.putImageData(imageData, 0, 0);
});

// Current canvas for operations (set by web_graph_flush_to_canvas)
static char current_canvas[64] = "canvas";

// Graph management functions
void graph_init(graph_t* g, const uint32_t* buffer, int32_t w, int32_t h) {
    g->buffer = (uint32_t*)buffer;
    g->w = w;
    g->h = h;
    g->clip.x = 0;
    g->clip.y = 0;
    g->clip.w = w;
    g->clip.h = h;
    g->need_free = false;
}

graph_t* graph_new(uint32_t* buffer, int32_t w, int32_t h) {
    graph_t* g = (graph_t*)malloc(sizeof(graph_t));
    if (!g) return NULL;
    
    if (!buffer) {
        buffer = (uint32_t*)malloc(w * h * sizeof(uint32_t));
        if (!buffer) {
            free(g);
            return NULL;
        }
        g->need_free = true;
    }
    
    graph_init(g, buffer, w, h);
    return g;
}

void graph_free(graph_t* g) {
    if (!g) return;
    if (g->need_free && g->buffer) {
        free(g->buffer);
    }
    free(g);
}

// Clipping functions
void graph_set_clip(graph_t* g, int x, int y, int w, int h) {
    g->clip.x = x;
    g->clip.y = y;
    g->clip.w = w;
    g->clip.h = h;
}

void graph_unset_clip(graph_t* g) {
    g->clip.x = 0;
    g->clip.y = 0;
    g->clip.w = g->w;
    g->clip.h = g->h;
}

bool graph_insect(graph_t* g, grect_t* r) {
    if (!r) return false;
    
    int x1 = (r->x > g->clip.x) ? r->x : g->clip.x;
    int y1 = (r->y > g->clip.y) ? r->y : g->clip.y;
    int x2 = ((r->x + r->w) < (g->clip.x + g->clip.w)) ? (r->x + r->w) : (g->clip.x + g->clip.w);
    int y2 = ((r->y + r->h) < (g->clip.y + g->clip.h)) ? (r->y + r->h) : (g->clip.y + g->clip.h);
    
    if (x1 >= x2 || y1 >= y2) return false;
    
    r->x = x1;
    r->y = y1;
    r->w = x2 - x1;
    r->h = y2 - y1;
    return true;
}

// Drawing functions
void graph_pixel(graph_t* g, int32_t x, int32_t y, uint32_t color) {
    if (x < g->clip.x || x >= g->clip.x + g->clip.w ||
        y < g->clip.y || y >= g->clip.y + g->clip.h) return;
    
    if (g->buffer && x >= 0 && x < g->w && y >= 0 && y < g->h) {
        g->buffer[y * g->w + x] = color;
    }
}

void graph_pixel_safe(graph_t* g, int32_t x, int32_t y, uint32_t color) {
    graph_pixel(g, x, y, color);
}

uint32_t graph_get_pixel(graph_t* g, int32_t x, int32_t y) {
    if (!g->buffer || x < 0 || x >= g->w || y < 0 || y >= g->h) return 0;
    return g->buffer[y * g->w + x];
}

void graph_clear(graph_t* g, uint32_t color) {
    js_canvas_clear(current_canvas, color_r(color), color_g(color), color_b(color), color_a(color));
    
    if (g->buffer) {
        for (int i = 0; i < g->w * g->h; i++) {
            g->buffer[i] = color;
        }
    }
}

void graph_fill(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    grect_t r = {x, y, w, h};
    if (!graph_insect(g, &r)) return;
    
    js_canvas_fill_rect(current_canvas, r.x, r.y, r.w, r.h, 
                        color_r(color), color_g(color), color_b(color), color_a(color));
    
    if (g->buffer) {
        for (int py = r.y; py < r.y + r.h; py++) {
            for (int px = r.x; px < r.x + r.w; px++) {
                g->buffer[py * g->w + px] = color;
            }
        }
    }
}

void graph_box(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    graph_line(g, x, y, x + w - 1, y, color);         // top
    graph_line(g, x, y + h - 1, x + w - 1, y + h - 1, color); // bottom
    graph_line(g, x, y, x, y + h - 1, color);         // left
    graph_line(g, x + w - 1, y, x + w - 1, y + h - 1, color); // right
}

void graph_line(graph_t* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
    js_canvas_draw_line(current_canvas, x1, y1, x2, y2,
                        color_r(color), color_g(color), color_b(color), color_a(color));
}

void graph_circle(graph_t* g, int32_t x, int32_t y, int32_t radius, uint32_t color) {
    js_canvas_draw_circle(current_canvas, x, y, radius,
                          color_r(color), color_g(color), color_b(color), color_a(color), 0);
}

void graph_fill_circle(graph_t* g, int32_t x, int32_t y, int32_t radius, uint32_t color) {
    js_canvas_draw_circle(current_canvas, x, y, radius,
                          color_r(color), color_g(color), color_b(color), color_a(color), 1);
}

void graph_draw_text(graph_t* g, int32_t x, int32_t y, const char* text, 
                     font_t* font, int32_t size, uint32_t color) {
    const char* font_name = font ? font->name : "Arial";
    js_canvas_draw_text(current_canvas, x, y, text, font_name, size,
                        color_r(color), color_g(color), color_b(color), color_a(color));
}

void graph_draw_text_font(graph_t* g, int32_t x, int32_t y, const char* text,
                          font_t* font, int32_t size, uint32_t color) {
    graph_draw_text(g, x, y, text, font, size, color);
}

// Blitting functions (simplified)
void graph_blt(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
               graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
    // Simple pixel-by-pixel copy for now
    if (!src->buffer || !dst->buffer) return;
    
    for (int y = 0; y < sh && dy + y < dst->h; y++) {
        for (int x = 0; x < sw && dx + x < dst->w; x++) {
            if (sx + x < src->w && sy + y < src->h) {
                uint32_t pixel = src->buffer[(sy + y) * src->w + (sx + x)];
                dst->buffer[(dy + y) * dst->w + (dx + x)] = pixel;
            }
        }
    }
}

void graph_blt_alpha(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
                     graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
    // Alpha blending implementation
    graph_blt(src, sx, sy, sw, sh, dst, dx, dy, dw, dh); // simplified for now
}

void web_graph_flush_to_canvas(graph_t* g, const char* canvas_id) {
    strncpy(current_canvas, canvas_id, sizeof(current_canvas) - 1);
    current_canvas[sizeof(current_canvas) - 1] = '\0';
    
    if (g->buffer) {
        js_canvas_flush_buffer(canvas_id, g->buffer, g->w, g->h);
    }
}

bool grect_insect(const grect_t* src, grect_t* dst) {
    if (!src || !dst) return false;
    
    int x1 = (src->x > dst->x) ? src->x : dst->x;
    int y1 = (src->y > dst->y) ? src->y : dst->y;
    int x2 = ((src->x + src->w) < (dst->x + dst->w)) ? (src->x + src->w) : (dst->x + dst->w);
    int y2 = ((src->y + src->h) < (dst->y + dst->h)) ? (src->y + src->h) : (dst->y + dst->h);
    
    if (x1 >= x2 || y1 >= y2) return false;
    
    dst->x = x1;
    dst->y = y1;
    dst->w = x2 - x1;
    dst->h = y2 - y1;
    return true;
}