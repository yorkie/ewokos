#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>

// EwokOS WASM Demo - Simplified version showing basic functionality
// Author: Misa.Z (misa.zhu@gmail.com)

typedef struct {
    char name[32];
    int pid;
    int status;
} process_t;

static process_t processes[8];
static int process_count = 0;
static int current_tick = 0;

// Console output buffer
static char console_buffer[4096];
static int console_pos = 0;

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
    ewokos_print("\nEwokOS WASM Demo - Running in Browser!\n");
    ewokos_print("Author: Misa.Z (misa.zhu@gmail.com)\n\n");
}

void ewokos_show_config() {
    ewokos_print("System Configuration:\n");
    ewokos_print("  Machine:             wasm-demo\n");
    ewokos_print("  Architecture:        wasm32\n");
    ewokos_print("  Memory:              16 MB\n");
    ewokos_print("  Max Processes:       8\n");
    ewokos_print("  Running in:          Browser (WebAssembly)\n");
    ewokos_print("-----------------------------------------------------\n\n");
}

int ewokos_create_process(const char* name) {
    if (process_count >= 8) return -1;
    
    strcpy(processes[process_count].name, name);
    processes[process_count].pid = process_count + 1;
    processes[process_count].status = 1; // running
    
    char msg[100];
    sprintf(msg, "Created process %d: %s\n", processes[process_count].pid, name);
    ewokos_print(msg);
    
    return process_count++;
}

void ewokos_list_processes() {
    ewokos_print("Process List:\n");
    ewokos_print("PID  NAME         STATUS\n");
    ewokos_print("---  -----------  ------\n");
    
    for (int i = 0; i < process_count; i++) {
        char line[80];
        sprintf(line, "%-3d  %-11s  %s\n", 
               processes[i].pid, 
               processes[i].name,
               processes[i].status ? "running" : "stopped");
        ewokos_print(line);
    }
    ewokos_print("\n");
}

void ewokos_scheduler_tick() {
    current_tick++;
    if (current_tick % 100 == 0) {
        char msg[50];
        sprintf(msg, "Scheduler tick: %d\n", current_tick);
        ewokos_print(msg);
    }
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
        ewokos_print("Available commands:\n");
        ewokos_print("  ps       - list processes\n");
        ewokos_print("  info     - show system info\n");
        ewokos_print("  create   - create demo process\n");
        ewokos_print("  tick     - scheduler tick\n");
        ewokos_print("  clear    - clear console\n");
        ewokos_print("  help     - show this help\n\n");
    } else if (strcmp(cmd, "info") == 0) {
        ewokos_show_config();
    } else if (strcmp(cmd, "create") == 0) {
        char pname[20];
        sprintf(pname, "proc_%d", process_count);
        ewokos_create_process(pname);
    } else if (strcmp(cmd, "tick") == 0) {
        ewokos_scheduler_tick();
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
    ewokos_show_config();
    
    // Create some initial processes
    ewokos_create_process("init");
    ewokos_create_process("kthreadd");
    ewokos_create_process("shell");
    
    ewokos_print("EwokOS Demo initialized. Type 'help' for commands.\n\n");
    
    return 0;
}