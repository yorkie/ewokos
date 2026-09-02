#include <dev/sd.h>
#include <stdint.h>

extern int32_t wasm_host_block_read(uint32_t sector, void *buffer,
        uint32_t count);

static int32_t pending_sector = -1;

int32_t sd_init(void) {
    pending_sector = -1;
    return 0;
}

int32_t sd_dev_read(int32_t sector) {
    pending_sector = sector;
    return sector < 0 ? -1 : 0;
}

int32_t sd_dev_read_done(void *buffer) {
    int32_t sector = pending_sector;

    pending_sector = -1;
    if(sector < 0)
        return -1;
    return wasm_host_block_read((uint32_t)sector, buffer, 1);
}

int32_t sd_dev_read_blocks(int32_t sector, void *buffer, uint32_t count) {
    if(sector < 0)
        return -1;
    return wasm_host_block_read((uint32_t)sector, buffer, count);
}

int32_t sd_dev_write(int32_t sector, const void *buffer) {
    (void)sector;
    (void)buffer;
    return -1;
}

int32_t sd_dev_write_done(void) {
    return -1;
}

void sd_dev_handle(void) {
}
