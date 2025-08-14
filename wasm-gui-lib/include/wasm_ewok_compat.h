#ifndef WASM_EWOK_COMPAT_H
#define WASM_EWOK_COMPAT_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// WebAssembly compatibility layer for EwokOS
// Provides stubs and replacements for EwokOS-specific functionality

// Basic math functions
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Process and IPC stubs
typedef struct {
    uint8_t* data;
    int32_t size;
    int32_t offset;
} proto_t;

typedef struct {
    proto_t* (*init)(proto_t* proto);
    void (*clear)(proto_t* proto);
    proto_t* (*addi)(proto_t* proto, int32_t i);
    proto_t* (*format)(proto_t* proto, const char* fmt, ...);
} proto_func_t;

extern proto_func_t* PF;

// Device control stubs
static inline int dev_cntl_by_pid(int pid, int cmd, void* in, void* out) { return -1; }
static inline void proc_block_by(int pid, int type) {}
static inline int proc_get_uuid(int pid) { return 1; }

// VFS stubs
static inline int vfs_fcntl(int fd, int cmd, void* in, void* out) { return -1; }
static inline int vfs_fcntl_wait(int fd, int cmd, void* in) { return -1; }

// Shared memory stubs
static inline int shmget(int key, size_t size, int flags) { return -1; }
static inline void* shmat(int id, const void* addr, int flags) { return NULL; }
static inline int shmdt(const void* addr) { return 0; }

// Thread stubs
#include <pthread.h>

// Basic string/path utilities
static inline const char* get_relative_name(const char* path) {
    const char* p = strrchr(path, '/');
    return p ? p + 1 : path;
}

#endif