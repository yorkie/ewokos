#ifndef WEB_GRAPH_H
#define WEB_GRAPH_H

#include "wasm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Graph management
void     graph_init(graph_t* g, const uint32_t* buffer, int32_t w, int32_t h);
graph_t* graph_new(uint32_t* buffer, int32_t w, int32_t h);
void     graph_free(graph_t* g);
graph_t* graph_dup(graph_t* g);

// Clipping
void     graph_set_clip(graph_t* g, int x, int y, int w, int h);
void     graph_unset_clip(graph_t* g);
bool     graph_insect(graph_t* g, grect_t* r);

// Basic drawing
void     graph_pixel(graph_t* g, int32_t x, int32_t y, uint32_t color);
void     graph_pixel_safe(graph_t* g, int32_t x, int32_t y, uint32_t color);
uint32_t graph_get_pixel(graph_t* g, int32_t x, int32_t y);
void     graph_clear(graph_t* g, uint32_t color);

// Shapes
void     graph_box(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void     graph_fill(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void     graph_line(graph_t* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
void     graph_circle(graph_t* g, int32_t x, int32_t y, int32_t radius, uint32_t color);
void     graph_fill_circle(graph_t* g, int32_t x, int32_t y, int32_t radius, uint32_t color);

// Blitting
void     graph_blt(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
                   graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh);
void     graph_blt_alpha(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
                         graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha);

// Text rendering (will use web fonts)
void     graph_draw_text(graph_t* g, int32_t x, int32_t y, const char* text, 
                         font_t* font, int32_t size, uint32_t color);
void     graph_draw_text_font(graph_t* g, int32_t x, int32_t y, const char* text,
                              font_t* font, int32_t size, uint32_t color);

// Web-specific functions
void     web_graph_flush_to_canvas(graph_t* g, const char* canvas_id);

// Utility functions
bool grect_insect(const grect_t* src, grect_t* dst);

#ifdef __cplusplus
}
#endif

#endif // WEB_GRAPH_H