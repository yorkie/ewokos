#ifndef TINYJSON_H
#define TINYJSON_H

#include "../ewoksys_wasm_compat.h"

// Minimal tinyjson compatibility - not needed for graphics
typedef struct {
    char* data;
} json_t;

static inline json_t* json_new(void) { return NULL; }
static inline void json_free(json_t* js) {}
static inline const char* json_get_str(json_t* js, const char* key) { return ""; }
static inline int json_get_int(json_t* js, const char* key) { return 0; }

#endif // TINYJSON_H