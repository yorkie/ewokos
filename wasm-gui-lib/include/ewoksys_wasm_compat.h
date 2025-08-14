#ifndef EWOKSYS_WASM_COMPAT_H
#define EWOKSYS_WASM_COMPAT_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// ===== WebAssembly compatibility layer for EwokOS =====
// This header provides WebAssembly-compatible implementations
// for all EwokOS-specific functionality to allow compiling
// original EwokOS source files without modification.

// Basic constants and types
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FS_FULL_NAME_MAX 1024

// ===== ewoksys/ipc.h compatibility =====
typedef struct proto_s {
    uint8_t* data;
    int32_t size;
    int32_t offset;
    
    // Method pointers for chaining (used by original code)
    struct proto_s* (*addi)(struct proto_s* proto, int32_t i);
    struct proto_s* (*adds)(struct proto_s* proto, const char* s);
    struct proto_s* (*format)(struct proto_s* proto, const char* fmt, ...);
} proto_t;

typedef struct {
    proto_t* (*init)(proto_t* proto);
    void (*clear)(proto_t* proto);
    proto_t* (*addi)(proto_t* proto, int32_t i);
    proto_t* (*format)(proto_t* proto, const char* fmt, ...);
    proto_t* (*adds)(proto_t* proto, const char* s);
    int32_t (*geti)(proto_t* proto);
    char* (*gets)(proto_t* proto);
} proto_func_t;

extern proto_func_t* PF;

// Additional proto functions used by original x.c
static inline int proto_read_int(proto_t* proto) { return 0; }
static inline void proto_read_to(proto_t* proto, void* data, int size) {}
static inline void proto_add_int(proto_t* proto, int32_t i) {}
static inline void proto_add_str(proto_t* proto, const char* s) {}

// ===== ewoksys/vfs.h compatibility =====
static inline int vfs_fcntl(int fd, int cmd, void* in, void* out) { return -1; }
static inline int vfs_fcntl_wait(int fd, int cmd, void* in) { return -1; }
static inline int vfs_file_name(const char* fname, char* name, int len) { 
    strncpy(name, fname, len-1);
    name[len-1] = 0;
    return 0;
}

// ===== ewoksys/syscall.h compatibility =====
static inline int ipc_call(int to_pid, int cmd, void* data, int32_t size, void* ret_data, int32_t ret_size) { return -1; }
static inline int ipc_send(int to_pid, int cmd, void* data, int32_t size) { return -1; }

// ===== ewoksys/thread.h compatibility =====
static inline void proc_block_by(int pid, int type) {}
static inline void proc_usleep(uint32_t usec) {}

// ===== ewoksys/proc.h compatibility =====
static inline int proc_get_uuid(int pid) { return 1; }
static inline int proc_get_pid(const char* proc_name) { return -1; }
static inline int proc_getpid(void) { return 1; }

// ===== ewoksys/vdevice.h compatibility =====
static inline int dev_cntl_by_pid(int pid, int cmd, void* in, void* out) { return -1; }
static inline int dev_get_pid(const char* dev_name) { return -1; }

// ===== ewoksys/cmain.h compatibility =====
// Empty - no specific functions needed

// ===== ewoksys/basic_math.h compatibility =====
// Basic math functions are available in standard C library

// ===== ewoksys/utf8unicode.h compatibility =====
static inline uint32_t utf8_to_unicode(const char* utf8, int* bytes) {
    if (bytes) *bytes = 1;
    return (uint32_t)*utf8; // Simple ASCII fallback
}

// ===== ewoksys/mstr.h compatibility =====
typedef struct {
    char* cstr;
    uint32_t len;
    uint32_t max;
} str_t;

static inline str_t* str_new(const char* s) {
    str_t* ret = malloc(sizeof(str_t));
    if (!ret) return NULL;
    ret->len = s ? strlen(s) : 0;
    ret->max = ret->len + 1;
    ret->cstr = malloc(ret->max);
    if (!ret->cstr) {
        free(ret);
        return NULL;
    }
    if (s) strcpy(ret->cstr, s);
    else ret->cstr[0] = 0;
    return ret;
}

static inline void str_free(str_t* s) {
    if (s) {
        if (s->cstr) free(s->cstr);
        free(s);
    }
}

static inline const char* CS(str_t* s) {
    return s ? s->cstr : "";
}

// ===== Shared memory compatibility =====
static inline int shmget(int key, size_t size, int flags) { return -1; }
static inline void* shmat(int id, const void* addr, int flags) { return NULL; }
static inline int shmdt(const void* addr) { return 0; }

// ===== Signal handling compatibility =====
// signal is provided by standard library, don't redefine it

// ===== Environment compatibility =====
// setenv and getenv are provided by standard library

// ===== WebAssembly-specific declarations =====
#ifdef __EMSCRIPTEN__
// Web event handling functions that will be called from JavaScript
void web_handle_mouse_event(int win_id, int type, int x, int y, int button);
void web_handle_key_event(int win_id, int type, int key, int code);
void web_x_init_canvas(int canvas_id, int width, int height);
void web_x_setup_event_handlers(void);
void web_graph_flush_to_canvas(int canvas_id, void* buffer, int width, int height);
#endif

// ===== Font system adaptation =====
// For WebAssembly, we'll use a simple font system that avoids conflicts
// with the original EwokOS font system which uses FreeType

#ifdef __EMSCRIPTEN__
// Only define these for WebAssembly to avoid conflicts with original font.h

typedef struct {
    char name[64];
    int size;
} wasm_font_t;

// Simple font functions for WebAssembly
static inline wasm_font_t* wasm_font_new(void) {
    wasm_font_t* font = malloc(sizeof(wasm_font_t));
    if (font) {
        strcpy(font->name, "Arial");
        font->size = 16;
    }
    return font;
}

static inline void wasm_font_free(wasm_font_t* font) {
    if (font) free(font);
}

static inline int wasm_font_init(void) { return 0; }

static inline int wasm_font_load(wasm_font_t* font, const char* fname, int size) {
    if (!font) return -1;
    strncpy(font->name, fname, 63);
    font->name[63] = 0;
    font->size = size;
    return 0;
}

static inline void wasm_font_text_size(wasm_font_t* font, const char* text, int* w, int* h) {
    if (!font || !text || !w || !h) return;
    *w = strlen(text) * (font->size * 0.6); // Rough estimate
    *h = font->size;
}

#endif // __EMSCRIPTEN__

#endif // EWOKSYS_WASM_COMPAT_H