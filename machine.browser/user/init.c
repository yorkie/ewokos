#include <stdint.h>
#include <ewoksys/syscall.h>
#include <syscalls.h>

static const char command[] = "/sbin/init.wasm";
static const char message[] = "init.wasm: entered EwokOS userspace via SYS_KPRINT\n";
static const char step_message[] = "init.wasm: cooperative step\n";
static uint32_t stepped;
extern unsigned char __heap_base;

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
    int32_t pid = (int32_t)syscall1(SYS_GET_PID, (uint32_t)-1);

    syscall2(SYS_KPRINT, (uint32_t)message, sizeof(message) - 1);
    return pid;
}

int32_t ewok_step(void) {
    if(stepped == 0) {
        syscall2(SYS_KPRINT, (uint32_t)step_message,
            sizeof(step_message) - 1);
        stepped = 1;
    }
    return 0;
}
