#include <stdint.h>
#include <sd/sd.h>

extern int32_t wasm_host_block_read(uint32_t sector, void *buffer,
        uint32_t count);
extern int32_t wasm_host_block_write(uint32_t sector, const void *buffer,
        uint32_t count);
extern int32_t wasm_host_block_flush(void);

static int32_t wasm_sd_init(void) {
    return 0;
}

static int32_t wasm_sd_read(int32_t sector, void *buffer) {
    return sector < 0 ? -1 :
        wasm_host_block_read((uint32_t)sector, buffer, 1);
}

static int32_t wasm_sd_read_blocks(int32_t sector, void *buffer,
        uint32_t count) {
    return sector < 0 ? -1 :
        wasm_host_block_read((uint32_t)sector, buffer, count);
}

static int32_t wasm_sd_write(int32_t sector, const void *buffer) {
    return sector < 0 ? -1 :
        wasm_host_block_write((uint32_t)sector, buffer, 1);
}

static int32_t wasm_sd_write_blocks(int32_t sector, const void *buffer,
        uint32_t count) {
    return sector < 0 ? -1 :
        wasm_host_block_write((uint32_t)sector, buffer, count);
}

int bsp_sd_init(void) {
    int32_t result = sd_init_ex(wasm_sd_init, wasm_sd_read,
        wasm_sd_read_blocks, wasm_sd_write, wasm_sd_write_blocks,
        wasm_host_block_flush);
    if(result == 0)
        sd_enable_sector_buffer(0);
    return result;
}
