#include <stdint.h>
#include <string.h>
#include <ewoksys/vdevice.h>

extern int32_t wasm_host_tty_write(const void *data, uint32_t size);
extern int32_t wasm_host_tty_read(void *data, uint32_t size);

static vdevice_t tty_device;

static int tty_read(vdevice_t *dev, int fd, int from_pid, fsinfo_t *info,
        void *buf, int size, int offset, void *p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)info;
    (void)offset;
    (void)p;
    int32_t count = wasm_host_tty_read(buf, (uint32_t)size);
    return count == 0 ? VFS_ERR_RETRY : count;
}

static int tty_write(vdevice_t *dev, int fd, int from_pid, fsinfo_t *info,
        const void *buf, int size, int offset, void *p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)info;
    (void)offset;
    (void)p;
    return wasm_host_tty_write(buf, (uint32_t)size);
}

int ewok_service_init(void) {
    memset(&tty_device, 0, sizeof(tty_device));
    strcpy(tty_device.desc, "browser tty");
    tty_device.read = tty_read;
    tty_device.write = tty_write;
    return device_run(&tty_device, "/dev/tty0", FS_TYPE_CHAR, 0666, false);
}

int ewok_service_step(void) {
    return 0;
}
