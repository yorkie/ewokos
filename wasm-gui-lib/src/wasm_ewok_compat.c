#include "wasm_ewok_compat.h"
#include <stdarg.h>

// Protocol function implementations for WebAssembly

static proto_t* proto_init(proto_t* proto) {
    if (!proto) return NULL;
    proto->data = NULL;
    proto->size = 0;
    proto->offset = 0;
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
    if (!proto) return NULL;
    // Stub implementation
    return proto;
}

static proto_t* proto_format(proto_t* proto, const char* fmt, ...) {
    if (!proto) return NULL;
    // Stub implementation
    return proto;
}

static int proto_read_int(proto_t* proto) {
    return 0; // Stub
}

static void proto_read_to(proto_t* proto, void* data, size_t size) {
    // Stub
}

static proto_func_t proto_functions = {
    .init = proto_init,
    .clear = proto_clear,
    .addi = proto_addi,
    .format = proto_format
};

proto_func_t* PF = &proto_functions;