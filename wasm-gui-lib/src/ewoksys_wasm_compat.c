#include "ewoksys_wasm_compat.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// ===== Protocol functions implementation =====
static proto_t* proto_init(proto_t* proto) {
    if (!proto) return NULL;
    proto->data = NULL;
    proto->size = 0;
    proto->offset = 0;
    
    // Set up method pointers for chaining
    proto->addi = proto_addi;
    proto->adds = proto_adds;
    proto->format = proto_format;
    
    return proto;
}

static void proto_clear(proto_t* proto) {
    if (proto && proto->data) {
        free(proto->data);
        proto->data = NULL;
        proto->size = 0;
        proto->offset = 0;
    }
}

static proto_t* proto_addi(proto_t* proto, int32_t i) {
    // Stub - not needed for graphics
    return proto;
}

static proto_t* proto_format(proto_t* proto, const char* fmt, ...) {
    // Stub - not needed for graphics  
    return proto;
}

static proto_t* proto_adds(proto_t* proto, const char* s) {
    // Stub - not needed for graphics
    return proto;
}

static int32_t proto_geti(proto_t* proto) {
    // Stub - not needed for graphics
    return 0;
}

static char* proto_gets(proto_t* proto) {
    // Stub - not needed for graphics
    return "";
}

// Global protocol functions pointer
static proto_func_t proto_funcs = {
    .init = proto_init,
    .clear = proto_clear,
    .addi = proto_addi,
    .format = proto_format,
    .adds = proto_adds,
    .geti = proto_geti,
    .gets = proto_gets
};

proto_func_t* PF = &proto_funcs;

// ===== WebAssembly-specific implementations =====

#ifdef __EMSCRIPTEN__
// These will be called from JavaScript
EM_JS(void, js_init_canvas, (int canvas_id, int width, int height), {
    // JavaScript implementation will be in the demo
});

EM_JS(void, js_flush_to_canvas, (int canvas_id, void* buffer, int width, int height), {
    // JavaScript implementation will be in the demo
});

void web_x_init_canvas(int canvas_id, int width, int height) {
    js_init_canvas(canvas_id, width, height);
}

void web_graph_flush_to_canvas(int canvas_id, void* buffer, int width, int height) {
    js_flush_to_canvas(canvas_id, buffer, width, height);
}

void web_x_setup_event_handlers(void) {
    // Setup will be done from JavaScript
}

void web_handle_mouse_event(int win_id, int type, int x, int y, int button) {
    // Will be implemented when we get the original xwin working
}

void web_handle_key_event(int win_id, int type, int key, int code) {
    // Will be implemented when we get the original xwin working
}
#endif