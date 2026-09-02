#include <stdint.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/vfs.h>

extern int32_t wasm_host_key_read(void *buffer, uint32_t size);
extern int32_t wasm_host_key_available(void);
static vdevice_t browser_key_device;

static int key_read(vdevice_t *dev, int fd, int from_pid, fsinfo_t *info,
        void *buffer, int size, int offset, void *data) {
    (void)dev; (void)fd; (void)from_pid; (void)info; (void)offset; (void)data;
    int32_t count = wasm_host_key_read(buffer, (uint32_t)size);
    return count == 0 ? VFS_ERR_RETRY : count;
}

static uint32_t key_poll(vdevice_t *dev, int fd, int from_pid, fsinfo_t *info,
        void *data) {
    (void)dev; (void)fd; (void)from_pid; (void)info; (void)data;
    return wasm_host_key_available() > 0 ? VFS_EVT_RD : 0;
}

int ewok_service_init(void) {
    memset(&browser_key_device, 0, sizeof(browser_key_device));
    strcpy(browser_key_device.desc, "browser keyboard");
    browser_key_device.read = key_read;
    browser_key_device.check_poll_events = key_poll;
    return device_run(&browser_key_device, "/dev/keyb0", FS_TYPE_CHAR, 0444,
            false);
}

int ewok_service_step(void) {
    if(wasm_host_key_available() > 0)
        vfs_wakeup(browser_key_device.mnt_info.node, VFS_EVT_RD);
    return 0;
}
