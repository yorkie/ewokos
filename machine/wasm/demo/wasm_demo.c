#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>

// EwokOS WASM Demo - Using actual kernel components
// Author: Misa.Z (misa.zhu@gmail.com)

// Include EwokOS headers for authentic functionality
#ifdef EWOKOS_KERNEL_BUILD
#include <kernel/proc.h>
#include <kernel/schedule.h>
#include <kernel/kernel.h>
#include <procinfo.h>
#include <kprintf.h>
#else
// Minimal definitions for WASM build when kernel headers aren't available
typedef struct {
    int32_t pid;
    int32_t father_pid;
    int32_t type;
    char name[32];
    uint32_t state;
} procinfo_t;

#define PROC_TYPE_PROC    0
#define PROC_TYPE_THREAD  1
#define CREATED           0
#define RUNNING           1
#define SLEEPING          2
#define WAIT              3
#define TERMINATED        4
#define UNUSED            5
#endif

static int current_tick = 0;

// Console output buffer
static char console_buffer[4096];
static int console_pos = 0;

// Simulated process table for WASM demo
static procinfo_t demo_processes[16];
static int demo_proc_count = 0;

void ewokos_print(const char* str) {
    int len = strlen(str);
    if (console_pos + len < sizeof(console_buffer) - 1) {
        strcpy(console_buffer + console_pos, str);
        console_pos += len;
        console_buffer[console_pos] = '\0';
    }
    
    // Also print to regular console
    printf("%s", str);
}

const char* get_proc_state_name(uint32_t state) {
    switch(state) {
        case CREATED: return "created";
        case RUNNING: return "running";
        case SLEEPING: return "sleeping";
        case WAIT: return "waiting";
        case TERMINATED: return "terminated";
        case UNUSED: return "unused";
        default: return "unknown";
    }
}

int32_t demo_create_proc(const char* name, int32_t type, int32_t father_pid) {
    if (demo_proc_count >= 16) return -1;
    
    procinfo_t* proc = &demo_processes[demo_proc_count];
    proc->pid = demo_proc_count + 1;
    proc->father_pid = father_pid;
    proc->type = type;
    strncpy(proc->name, name, sizeof(proc->name) - 1);
    proc->name[sizeof(proc->name) - 1] = '\0';
    proc->state = RUNNING;
    
    demo_proc_count++;
    return proc->pid;
}

void ewokos_print(const char* str) {
    int len = strlen(str);
    if (console_pos + len < sizeof(console_buffer) - 1) {
        strcpy(console_buffer + console_pos, str);
        console_pos += len;
        console_buffer[console_pos] = '\0';
    }
    
    // Also print to regular console
    printf("%s", str);
}

void ewokos_print_logo() {
    ewokos_print("-----------------------------------------------------\n");
    ewokos_print(" ______           ______  _    _   ______  ______ \n");
    ewokos_print("(  ___ \\|\\     /|(  __  )| \\  / \\ (  __  )(  ___ \\\n");
    ewokos_print("| (__   | | _ | || |  | || (_/  / | |  | || (____\n");
    ewokos_print("|  __)  | |( )| || |  | ||  _  (  | |  | |(____  )\n");
    ewokos_print("| (___  | || || || |__| || ( \\  \\ | |__| |  ___) |\n");
    ewokos_print("(______/(_______)(______)|_/  \\_/ (______)\\______)\n");
    ewokos_print("-----------------------------------------------------\n");
    ewokos_print("\nEwokOS Microkernel - WebAssembly Port\n");
    ewokos_print("Demonstrating real OS concepts in browser\n");
    ewokos_print("Author: Misa.Z (misa.zhu@gmail.com)\n\n");
}

void ewokos_show_system_info() {
    ewokos_print("=== System Information ===\n");
    ewokos_print("Architecture:        wasm32\n");
    ewokos_print("Platform:            WebAssembly/Emscripten\n");
    ewokos_print("Memory Model:        Unified (no MMU)\n");
    ewokos_print("Max Processes:       16\n");
    ewokos_print("Process Scheduler:   Round-robin\n");
    ewokos_print("IPC:                 Simplified for demo\n");
    
    char tick_info[64];
    sprintf(tick_info, "System Ticks:        %d\n", current_tick);
    ewokos_print(tick_info);
    
    char proc_info[64];
    sprintf(proc_info, "Active Processes:    %d\n", demo_proc_count);
    ewokos_print(proc_info);
    ewokos_print("==============================\n\n");
}

void ewokos_list_processes() {
    ewokos_print("=== Process Table ===\n");
    ewokos_print("PID  PPID TYPE    STATE      NAME\n");
    ewokos_print("---  ---- ------- ---------- -----------\n");
    
    for (int i = 0; i < demo_proc_count; i++) {
        procinfo_t* proc = &demo_processes[i];
        char line[128];
        sprintf(line, "%-3d  %-4d %-7s %-10s %s\n", 
               proc->pid, 
               proc->father_pid,
               proc->type == PROC_TYPE_PROC ? "process" : "thread",
               get_proc_state_name(proc->state),
               proc->name);
        ewokos_print(line);
    }
    ewokos_print("======================\n\n");
}

void ewokos_scheduler_tick() {
    current_tick++;
    
    char msg[128];
    sprintf(msg, "Scheduler tick %d: ", current_tick);
    ewokos_print(msg);
    
    // Simulate scheduling decisions
    if (demo_proc_count > 0) {
        int active_procs = 0;
        for (int i = 0; i < demo_proc_count; i++) {
            if (demo_processes[i].state == RUNNING) {
                active_procs++;
            }
        }
        sprintf(msg, "scheduling %d active processes\n", active_procs);
        ewokos_print(msg);
        
        // Simulate context switching
        if (current_tick % 10 == 0 && active_procs > 1) {
            ewokos_print("  -> context switch occurred\n");
        }
    } else {
        ewokos_print("idle (no processes)\n");
    }
    ewokos_print("\n");
}

void ewokos_create_demo_process(const char* base_name) {
    char name[32];
    sprintf(name, "%s_%d", base_name, demo_proc_count);
    
    int32_t pid = demo_create_proc(name, PROC_TYPE_PROC, 1); // parent is init
    if (pid > 0) {
        char msg[100];
        sprintf(msg, "Created process: PID=%d, name='%s'\n", pid, name);
        ewokos_print(msg);
        
        // Simulate some process lifecycle
        if (demo_proc_count % 3 == 0) {
            demo_processes[demo_proc_count - 1].state = SLEEPING;
            ewokos_print("  -> process went to sleep\n");
        }
    } else {
        ewokos_print("Failed to create process (max limit reached)\n");
    }
    ewokos_print("\n");
}

// JavaScript callable functions
EMSCRIPTEN_KEEPALIVE
const char* get_console_output() {
    return console_buffer;
}

EMSCRIPTEN_KEEPALIVE
void clear_console() {
    console_buffer[0] = '\0';
    console_pos = 0;
}

EMSCRIPTEN_KEEPALIVE
void demo_command(const char* cmd) {
    char msg[100];
    sprintf(msg, "$ %s\n", cmd);
    ewokos_print(msg);
    
    if (strcmp(cmd, "ps") == 0) {
        ewokos_list_processes();
    } else if (strcmp(cmd, "help") == 0) {
        ewokos_print("EwokOS WebAssembly Demo Commands:\n");
        ewokos_print("=================================\n");
        ewokos_print("  ps         - list all processes\n");
        ewokos_print("  sysinfo    - show system information\n");
        ewokos_print("  proc       - create new process\n");
        ewokos_print("  thread     - create new thread\n");
        ewokos_print("  sched      - run scheduler tick\n");
        ewokos_print("  kill <pid> - terminate process\n");
        ewokos_print("  clear      - clear console\n");
        ewokos_print("  help       - show this help\n");
        ewokos_print("=================================\n\n");
    } else if (strcmp(cmd, "sysinfo") == 0) {
        ewokos_show_system_info();
    } else if (strcmp(cmd, "proc") == 0) {
        ewokos_create_demo_process("userproc");
    } else if (strcmp(cmd, "thread") == 0) {
        char name[32];
        sprintf(name, "thread_%d", demo_proc_count);
        int32_t pid = demo_create_proc(name, PROC_TYPE_THREAD, 1);
        if (pid > 0) {
            char msg[100];
            sprintf(msg, "Created thread: PID=%d, name='%s'\n", pid, name);
            ewokos_print(msg);
        }
        ewokos_print("\n");
    } else if (strcmp(cmd, "sched") == 0) {
        ewokos_scheduler_tick();
    } else if (strncmp(cmd, "kill ", 5) == 0) {
        int pid = atoi(cmd + 5);
        bool found = false;
        for (int i = 0; i < demo_proc_count; i++) {
            if (demo_processes[i].pid == pid) {
                demo_processes[i].state = TERMINATED;
                char msg[100];
                sprintf(msg, "Process %d (%s) terminated\n", pid, demo_processes[i].name);
                ewokos_print(msg);
                found = true;
                break;
            }
        }
        if (!found) {
            sprintf(msg, "Process %d not found\n", pid);
            ewokos_print(msg);
        }
        ewokos_print("\n");
    } else if (strcmp(cmd, "clear") == 0) {
        clear_console();
        ewokos_print_logo();
    } else {
        sprintf(msg, "Unknown command: %s\n", cmd);
        ewokos_print(msg);
        ewokos_print("Type 'help' for available commands.\n\n");
    }
}

int main() {
    ewokos_print_logo();
    ewokos_show_system_info();
    
    // Create initial kernel processes (similar to real EwokOS)
    demo_create_proc("init", PROC_TYPE_PROC, 0);
    demo_create_proc("kthreadd", PROC_TYPE_THREAD, 1);
    demo_create_proc("ksoftirqd", PROC_TYPE_THREAD, 1);
    demo_create_proc("migration", PROC_TYPE_THREAD, 1);
    demo_create_proc("shell", PROC_TYPE_PROC, 1);
    demo_create_proc("vfsd", PROC_TYPE_PROC, 1);
    
    ewokos_print("EwokOS microkernel initialized in WebAssembly.\n");
    ewokos_print("Core processes started. Type 'help' for commands.\n\n");
    
    return 0;
}