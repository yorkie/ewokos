#include <dev/uart.h>
#include <kernel/irq.h>
#include <kernel/system.h>

// WASM imports for UART simulation
extern void wasm_debug_print(const char* str, uint32_t len);
extern int wasm_uart_has_data(void);
extern char wasm_uart_read_char(void);

// Initialize UART for WASM
void uart_arch_init(int baud) {
    (void)baud;
    // WASM UART is always ready
}

// Write character to UART
void uart_arch_write(char c) {
    wasm_debug_print(&c, 1);
}

// Read character from UART
char uart_arch_read(void) {
    if (wasm_uart_has_data()) {
        return wasm_uart_read_char();
    }
    return 0;
}

// Check if UART has data available
int uart_arch_ready_to_recv(void) {
    return wasm_uart_has_data();
}

// Check if UART is ready to send
int uart_arch_ready_to_send(void) {
    return 1; // Always ready in WASM
}

// Enable UART interrupts
void uart_arch_enable_irq(void) {
    // WASM host will trigger UART interrupts when data arrives
}

// Disable UART interrupts
void uart_arch_disable_irq(void) {
    // Cannot disable in WASM
}

// Clear UART interrupt
void uart_arch_clear_irq(void) {
    // No hardware interrupt to clear
}

// Set UART configuration
void uart_arch_config(int data_bits, int stop_bits, int parity) {
    (void)data_bits;
    (void)stop_bits;
    (void)parity;
    // WASM UART configuration is fixed
}

// WASM-specific function to trigger UART interrupt from host
void wasm_uart_interrupt(void) {
    // This will be called by the WASM host when UART data arrives
    uart_irq_handler();
}