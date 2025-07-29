#include <dev/uart.h>
#include <stdint.h>
#include <emscripten.h>
#include <stdio.h>

int32_t uart_dev_init(uint32_t baud) {
    (void)baud; // WASM doesn't use traditional UART
    // UART output is handled through JavaScript console
    return 0;
}

int32_t uart_write(const void* data, uint32_t size) {
    const char* str = (const char*)data;
    
    // For now, just use printf which will go to console in emscripten
    for (uint32_t i = 0; i < size; i++) {
        if (str[i] != 0) {
            printf("%c", str[i]);
        }
    }
    
    return size;
}

int32_t uart_read(uint32_t id, void* data, uint32_t size) {
    (void)id; (void)data; (void)size;
    // Input handling would need to be implemented with JavaScript callbacks
    return 0;
}

// Keyboard input handler - called from JavaScript
EMSCRIPTEN_KEEPALIVE
void wasm_keyboard_input(int keycode) {
    // This could be used to feed input to the kernel
    // For now, just ignore it
    (void)keycode;
}