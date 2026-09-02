#include <stdint.h>
#include <ewoksys/syscall.h>
#include <syscalls.h>
#include <sysinfo.h>

static const char command[] = "/sbin/worker.wasm";
static const char message[] = "worker.wasm: entered isolated EwokOS process\n";
static const char step_message[] = "worker.wasm: cooperative step\n";
static const char sysinfo_message[] =
    "worker.wasm: read sysinfo through software MMU\n";
static uint32_t stepped;
static sys_info_t system_info;
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
        if((int32_t)syscall1(SYS_GET_SYS_INFO,
                (uint32_t)&system_info) != 0)
            return -1;
        syscall2(SYS_KPRINT, (uint32_t)step_message,
            sizeof(step_message) - 1);
        syscall2(SYS_KPRINT, (uint32_t)sysinfo_message,
            sizeof(sysinfo_message) - 1);
        stepped = 1;
    }
    return 0;
}

uint32_t ewok_memory_mb(void) {
    return system_info.total_phy_mem_size / (1024u * 1024u);
}
