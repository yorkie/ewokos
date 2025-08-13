#include "ext2read.h"
#include <stddef.h>
#include <kprintf.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <mm/kmalloc.h>
#include <dev/sd.h>
#include <kstring.h>

// WASM imports for loading init process
extern int wasm_load_init_process(const char* path, void** elf_data, int* size);

int32_t load_init_proc(void) {
    const char* prog = "/sbin/init";
    void* elf_data;
    int sz;

    // Try to load from WASM host first
    if (wasm_load_init_process(prog, &elf_data, &sz) == 0 && elf_data != NULL) {
        proc_t *proc = proc_create(TASK_TYPE_PROC, NULL);
        strcpy(proc->info.cmd, prog);
        proc->info.uid = -1;
        page_dir_entry_t *vm = proc->space->vm;
        set_translation_table_base((uint32_t)V2P(vm));
        
        // Copy the ELF data since we need to free it later
        char* elf = (char*)kmalloc(sz);
        if (elf == NULL) {
            return -1;
        }
        memcpy(elf, elf_data, sz);
        
        int32_t res = proc_load_elf(proc, elf, sz);
        kfree(elf);
        return res;
    }
    
    // Fallback to SD card loading if available
    char* elf = sd_read_ext2(prog, &sz);
    if(elf != NULL) {
        proc_t *proc = proc_create(TASK_TYPE_PROC, NULL);
        strcpy(proc->info.cmd, prog);
        proc->info.uid = -1;
        page_dir_entry_t *vm = proc->space->vm;
        set_translation_table_base((uint32_t)V2P(vm));
        int32_t res = proc_load_elf(proc, elf, sz);
        kfree(elf);
        return res;
    }
    return -1;
}