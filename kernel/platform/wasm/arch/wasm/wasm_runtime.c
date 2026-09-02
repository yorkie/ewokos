#include <kernel/context.h>
#include <kernel/kernel.h>
#include <kernel/ipc.h>
#include <kernel/proc.h>
#include <kernel/svc.h>
#include <kernel/system.h>
#include <kstring.h>
#include <mm/mmu.h>
#include <procinfo.h>
#include <syscalls.h>
#include <sysinfo.h>
#include <stdint.h>

static uint32_t wasm_second_usec;

static int32_t wasm_user_range_mapped(proc_t *proc, uint32_t address,
        uint32_t size) {
    uint32_t current;
    uint32_t end;

    if(proc == NULL || proc->space == NULL || size == 0)
        return 0;
    end = address + size;
    if(end < address || end > KERNEL_BASE)
        return 0;
    current = ALIGN_DOWN(address, PAGE_SIZE);
    end = ALIGN_UP(end, PAGE_SIZE);
    while(current < end) {
        page_table_entry_t *entry =
            get_page_table_entry(proc->space->vm, current);
        if(entry == NULL || (entry->value & WASM_PTE_USER) == 0)
            return 0;
        current += PAGE_SIZE;
    }
    return 1;
}

int32_t wasm_kernel_spawn(const char *command, uint32_t command_len,
        uint32_t module_base, uint32_t module_size, uint32_t heap_base) {
    proc_t *proc;
    uint32_t command_addr = (uint32_t)command;
    uint32_t length;

    if(command_len == 0 || module_size == 0 || module_base >= KERNEL_BASE ||
            module_base + module_size < module_base ||
            module_base + module_size > KERNEL_BASE ||
            heap_base < module_base || heap_base >= module_base + module_size ||
            command_addr < module_base || command_addr + command_len < command_addr ||
            command_addr + command_len > module_base + module_size)
        return -1;
    proc = proc_create(TASK_TYPE_PROC, NULL);
    if(proc == NULL)
        return -1;
    map_pages_size(proc->space->vm, module_base, module_base, module_size,
        AP_RW_RW, PTE_ATTR_WRBACK);
    if(!wasm_user_range_mapped(proc, module_base, module_size) ||
            !wasm_user_range_mapped(proc, (uint32_t)command, command_len))
        return -1;
    /* Native ELF loading advances these fields past the last load segment.
     * A wasm module is registered rather than copied by proc_load_elf(), so
     * establish the same heap boundary explicitly. */
    proc->space->rw_heap_base = module_base;
    proc->space->heap_size = ALIGN_UP(heap_base, PAGE_SIZE);
    proc->space->heap_used = proc->space->heap_size;
    proc->space->malloc_base = proc->space->heap_size;
    proc->space->heap_limit = module_base + module_size;
    length = command_len;
    if(length >= sizeof(proc->info.cmd))
        length = sizeof(proc->info.cmd) - 1;
    memcpy(proc->info.cmd, command, length);
    proc->info.cmd[length] = 0;
    proc->info.uid = 0;
    proc->info.gid = 0;
    proc->info.state = RUNNING;
    set_current_proc(proc);
    return proc->info.pid;
}

int32_t wasm_kernel_activate(int32_t pid) {
    proc_t *next = proc_get(pid);
    proc_t *current = get_current_proc();

    if(next == NULL || next->info.type != TASK_TYPE_PROC ||
            next->info.state == UNUSED || next->info.state == ZOMBIE)
        return -1;
    if(current != NULL && current != next && current->info.state == RUNNING)
        current->info.state = READY;
    next->info.state = RUNNING;
    set_current_proc(next);
    return 0;
}

int32_t wasm_kernel_set_system_process(int32_t pid) {
    proc_t *proc = proc_get(pid);
    if(proc == NULL || proc->info.type != TASK_TYPE_PROC)
        return -1;
    proc->info.uid = (uint32_t)-1;
    proc->info.gid = (uint32_t)-1;
    return 0;
}

int32_t wasm_kernel_runnable(int32_t pid) {
    proc_t *proc = proc_get(pid);

    if(proc == NULL)
        return 0;
    return proc->info.state == READY || proc->info.state == RUNNING;
}

void wasm_kernel_tick(uint32_t elapsed_usec) {
    if(elapsed_usec > 1000000u)
        elapsed_usec = 1000000u;
    _kernel_info.uptime_usec += elapsed_usec;
    wasm_second_usec += elapsed_usec;
    while(wasm_second_usec >= 1000000u) {
        wasm_second_usec -= 1000000u;
        _kernel_info.uptime_sec++;
        renew_kernel_sec();
    }
    renew_kernel_tic(elapsed_usec);
}

uint64_t wasm_kernel_uptime_usec(void) {
    return _kernel_info.uptime_usec;
}

uint32_t wasm_kernel_physical_limit(void) {
    return (uint32_t)(_sys_info.phy_offset + _sys_info.total_phy_mem_size);
}

uint32_t wasm_kernel_module_limit(void) {
    return (uint32_t)KMALLOC_VM_BASE;
}

uint32_t wasm_kernel_shm_base(void) {
    return (uint32_t)SHM_BASE;
}

uint32_t wasm_kernel_stack_bottom(void) {
    return (uint32_t)USER_STACK_BOTTOM;
}

uint32_t wasm_kernel_base(void) {
    return (uint32_t)KERNEL_BASE;
}

int32_t wasm_kernel_current_pid(void) {
    proc_t *proc = get_current_proc();
    return proc == NULL ? -1 : proc->info.pid;
}

int32_t wasm_kernel_current_owner_pid(void) {
    proc_t *proc = get_current_proc();
    proc_t *owner = proc == NULL ? NULL : proc_get_proc(proc);
    return owner == NULL ? -1 : owner->info.pid;
}

int32_t wasm_kernel_current_dispatchable(void) {
    proc_t *proc = get_current_proc();
    if(proc == NULL)
        return 0;
    if(proc->info.type == TASK_TYPE_THREAD)
        return 1;
    return proc_ipc_sync_serving(proc) ? 1 : 0;
}

uint32_t wasm_kernel_current_entry(void) {
    proc_t *proc = get_current_proc();
    return proc == NULL ? 0 : (uint32_t)proc->ctx.pc;
}

uint32_t wasm_kernel_current_arg(uint32_t index) {
    proc_t *proc = get_current_proc();
    if(proc == NULL || index >= 4)
        return 0;
    return (uint32_t)proc->ctx.gpr[index];
}

int32_t wasm_kernel_result(int32_t pid) {
    proc_t *proc = proc_get(pid);
    return proc == NULL ? -1 : (int32_t)proc->ctx.gpr[0];
}

int32_t wasm_kernel_syscall(int32_t code, uint32_t arg0, uint32_t arg1,
        uint32_t arg2) {
    proc_t *proc = get_current_proc();

    if(proc == NULL)
        return -1;
    if(code == SYS_KPRINT && !wasm_user_range_mapped(proc, arg0, arg1))
        return -1;
    if(code == SYS_GET_SYS_INFO &&
            !wasm_user_range_mapped(proc, arg0, sizeof(sys_info_t)))
        return -1;
    svc_handler(code, arg0, arg1, arg2, &proc->ctx);
    __irq_enable();
    return (int32_t)proc->ctx.gpr[0];
}
