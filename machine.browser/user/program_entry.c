#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <ewoksys/proc.h>
#include <ewoksys/signal.h>
#include <ewoksys/syscall.h>
#include <ewoksys/vfs.h>
#include <sysinfo.h>

#ifndef EWOK_WASM_COMMAND
#error "EWOK_WASM_COMMAND must name the program"
#endif
#ifndef EWOK_WASM_ARG0
#define EWOK_WASM_ARG0 EWOK_WASM_COMMAND
#endif
#ifndef EWOK_WASM_ARG1
#define EWOK_WASM_ARG1 0
#endif
#ifndef EWOK_WASM_ARG2
#define EWOK_WASM_ARG2 0
#endif
#ifndef EWOK_WASM_ARG3
#define EWOK_WASM_ARG3 0
#endif
#ifndef EWOK_WASM_ARG4
#define EWOK_WASM_ARG4 0
#endif
#ifndef EWOK_WASM_ARG5
#define EWOK_WASM_ARG5 0
#endif
#ifndef EWOK_WASM_ARG6
#define EWOK_WASM_ARG6 0
#endif

static const char command[] = EWOK_WASM_COMMAND;
static int32_t program_result = -1;
extern unsigned char __heap_base;

vsyscall_info_t *_vsyscall_info;
int _current_pid = -1;

extern void _libc_init(void);
extern int ewok_program_main(int argc, char **argv);

uint32_t ewok_init_command(void) { return (uint32_t)command; }
uint32_t ewok_init_command_size(void) { return sizeof(command) - 1; }
uint32_t ewok_module_base(void) { return EWOK_WASM_MODULE_BASE; }
uint32_t ewok_module_size(void) { return EWOK_WASM_MODULE_SIZE; }
uint32_t ewok_heap_base(void) { return (uint32_t)(uintptr_t)&__heap_base; }
int32_t ewok_program_result(void) { return program_result; }
int32_t ewok_start_stage(void) { return program_result == -1 ? 4 : 5; }

int32_t _start(void) {
    _current_pid = (int32_t)syscall1(SYS_GET_PID, (uint32_t)-1);
    _vsyscall_info = (vsyscall_info_t *)syscall0(SYS_GET_VSYSCALL_INFO);
    if(_current_pid < 0 || _vsyscall_info == 0)
        return -1;
    _libc_init();
    proc_init();
    sys_signal_init();
    vfs_init();

    int tty = open("/dev/tty0", O_RDWR);
    if(tty < 0)
        return -1;
    dup2(tty, 0);
    dup2(tty, 1);
    dup2(tty, 2);
    if(tty > 2)
        close(tty);

    char *argv[] = { EWOK_WASM_ARG0, EWOK_WASM_ARG1, EWOK_WASM_ARG2,
            EWOK_WASM_ARG3, EWOK_WASM_ARG4, EWOK_WASM_ARG5,
            EWOK_WASM_ARG6, 0 };
    int argc = 0;
    while(argv[argc] != 0)
        argc++;
    program_result = ewok_program_main(argc, argv);
    return program_result == 0 ? _current_pid : -1;
}

int32_t ewok_step(void) { return 0; }

int32_t ewok_dispatch(uint32_t entry, uint32_t arg0, uint32_t arg1) {
    void (*handler)(uint32_t, void *) =
        (void (*)(uint32_t, void *))(uintptr_t)entry;
    if(entry == 0)
        return -1;
    handler(arg0, (void *)(uintptr_t)arg1);
    return 0;
}
