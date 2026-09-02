#include <stdint.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/vfs.h>

extern int32_t wasm_host_mouse_read(void *buffer, uint32_t size);
extern int32_t wasm_host_mouse_available(void);
static vdevice_t browser_mouse_device;

static int mouse_read(vdevice_t *dev, int fd, int from_pid, fsinfo_t *info,
        void *buffer, int size, int offset, void *data) {
    (void)dev; (void)fd; (void)from_pid; (void)info; (void)offset; (void)data;
    int32_t count = wasm_host_mouse_read(buffer, (uint32_t)size);
    return count == 0 ? VFS_ERR_RETRY : count;
}

static uint32_t mouse_poll(vdevice_t *dev, int fd, int from_pid,
        fsinfo_t *info, void *data) {
    (void)dev; (void)fd; (void)from_pid; (void)info; (void)data;
    return wasm_host_mouse_available() >= 8 ? VFS_EVT_RD : 0;
}

int ewok_service_init(void) {
    memset(&browser_mouse_device, 0, sizeof(browser_mouse_device));
    strcpy(browser_mouse_device.desc, "browser mouse");
    browser_mouse_device.read = mouse_read;
    browser_mouse_device.check_poll_events = mouse_poll;
    return device_run(&browser_mouse_device, "/dev/mouse0", FS_TYPE_CHAR,
            0444, false);
}

int ewok_service_step(void) {
    if(wasm_host_mouse_available() >= 8)
        vfs_wakeup(browser_mouse_device.mnt_info.node, VFS_EVT_RD);
    return 0;
}
