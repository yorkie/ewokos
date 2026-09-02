#include <stdint.h>
#include <string.h>

#include <ewoksys/vdevice.h>
#include <ewoksys/vfs.h>

extern int32_t wasm_host_net_read(void *buffer, uint32_t size);
extern int32_t wasm_host_net_write(const void *buffer, uint32_t size);
extern int32_t wasm_host_net_available(void);

static vdevice_t browser_net_device;

static int net_read(vdevice_t *dev, int fd, int from_pid, fsinfo_t *info,
        void *buffer, int size, int offset, void *data) {
    (void)dev; (void)fd; (void)from_pid; (void)info; (void)offset; (void)data;
    int32_t count = wasm_host_net_read(buffer, (uint32_t)size);
    return count == 0 ? VFS_ERR_RETRY : count;
}

static int net_write(vdevice_t *dev, int fd, int from_pid, fsinfo_t *info,
        const void *buffer, int size, int offset, void *data) {
    (void)dev; (void)fd; (void)from_pid; (void)info; (void)offset; (void)data;
    return wasm_host_net_write(buffer, (uint32_t)size);
}

static int net_control(vdevice_t *dev, int from_pid, int cmd, proto_t *in,
        proto_t *out, void *data) {
    (void)dev; (void)from_pid; (void)in; (void)data;
    static const uint8_t mac[6] = {0x02, 0x45, 0x57, 0x4f, 0x4b, 0x01};
    if(cmd == 0) {
        PF->add(out, mac, sizeof(mac));
        return 0;
    }
    if(cmd == 1) {
        PF->addi(out, wasm_host_net_available());
        return 0;
    }
    return -1;
}

static uint32_t net_poll(vdevice_t *dev, int fd, int from_pid,
        fsinfo_t *info, void *data) {
    (void)dev; (void)fd; (void)from_pid; (void)info; (void)data;
    return (wasm_host_net_available() > 0 ? VFS_EVT_RD : 0) | VFS_EVT_WR;
}

int ewok_service_init(void) {
    memset(&browser_net_device, 0, sizeof(browser_net_device));
    strcpy(browser_net_device.desc, "browser ethernet");
    browser_net_device.read = net_read;
    browser_net_device.write = net_write;
    browser_net_device.dev_cntl = net_control;
    browser_net_device.check_poll_events = net_poll;
    return device_run(&browser_net_device, "/dev/eth0", FS_TYPE_CHAR, 0666,
            false);
}

int ewok_service_step(void) {
    if(wasm_host_net_available() > 0)
        vfs_wakeup(browser_net_device.mnt_info.node, VFS_EVT_RD);
    return 0;
}
