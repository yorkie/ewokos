#include <dev/sd.h>
#include <kernel/system.h>
#include <kstring.h>

// WASM imports for file system simulation
extern int wasm_sd_read_block(uint32_t block, void* buffer, uint32_t size);
extern int wasm_sd_write_block(uint32_t block, const void* buffer, uint32_t size);
extern uint32_t wasm_sd_get_size(void);

// Initialize SD card for WASM
int sd_arch_init(void) {
    // WASM SD is always ready
    return 0;
}

// Read block from SD card
int sd_arch_read_block(uint32_t block, void* buffer) {
    return wasm_sd_read_block(block, buffer, SD_BLOCK_SIZE);
}

// Write block to SD card
int sd_arch_write_block(uint32_t block, const void* buffer) {
    return wasm_sd_write_block(block, buffer, SD_BLOCK_SIZE);
}

// Get SD card size in blocks
uint32_t sd_arch_get_size(void) {
    return wasm_sd_get_size();
}

// Check if SD card is present
int sd_arch_present(void) {
    return 1; // Always present in WASM
}

// Check if SD card is write protected
int sd_arch_write_protected(void) {
    return 0; // Never write protected in WASM
}

// SD card reset
void sd_arch_reset(void) {
    // No reset needed for WASM
}

// Set SD clock speed
void sd_arch_set_clock(uint32_t speed) {
    (void)speed;
    // Clock speed is irrelevant in WASM
}

// Wait for SD card ready
int sd_arch_wait_ready(uint32_t timeout) {
    (void)timeout;
    return 0; // Always ready
}