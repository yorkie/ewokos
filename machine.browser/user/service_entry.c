#include <stdint.h>
#include <ewoksys/proc.h>
#include <ewoksys/signal.h>
#include <ewoksys/syscall.h>
#include <ewoksys/vfs.h>
#include <sysinfo.h>

#ifndef EWOK_WASM_COMMAND
#error "EWOK_WASM_COMMAND must name the service"
#endif

static const char command[] = EWOK_WASM_COMMAND;
static int32_t start_stage;
extern unsigned char __heap_base;

vsyscall_info_t *_vsyscall_info;
int _current_pid = -1;

extern void _libc_init(void);
extern int ewok_service_init(void);
extern int ewok_service_step(void);

uint32_t ewok_init_command(void) {
    return (uint32_t)command;
}

uint32_t ewok_init_command_size(void) {
    return sizeof(command) - 1;
}

uint32_t ewok_module_base(void) {
    return EWOK_WASM_MODULE_BASE;
}

uint32_t ewok_module_size(void) {
    return EWOK_WASM_MODULE_SIZE;
}

uint32_t ewok_heap_base(void) {
    return (uint32_t)(uintptr_t)&__heap_base;
}

int32_t _start(void) {
    start_stage = 1;
    _current_pid = (int32_t)syscall1(SYS_GET_PID, (uint32_t)-1);
    _vsyscall_info = (vsyscall_info_t *)syscall0(SYS_GET_VSYSCALL_INFO);
    if(_current_pid < 0 || _vsyscall_info == 0)
        return -1;

    start_stage = 2;
    _libc_init();
    start_stage = 3;
    proc_init();
    sys_signal_init();
    vfs_init();
    start_stage = 4;
    if(ewok_service_init() != 0)
        return -1;
    start_stage = 5;
    return _current_pid;
}

int32_t ewok_start_stage(void) {
    return start_stage;
}

int32_t ewok_step(void) {
    return ewok_service_step();
}

int32_t ewok_dispatch(uint32_t entry, uint32_t arg0, uint32_t arg1) {
    void (*handler)(uint32_t, void *) =
        (void (*)(uint32_t, void *))(uintptr_t)entry;
    if(entry == 0)
        return -1;
    handler(arg0, (void *)(uintptr_t)arg1);
    return 0;
}
