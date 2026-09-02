#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/mmio.h>
#include <ewoksys/dma.h>

static int power_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        void* buf, int size, int offset, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)info;
    (void)offset;
    (void)p;
    static int r = 10;

    uint8_t* data = (uint8_t *)buf;
    data[0] = 1;
    data[1] = 1;
    data[2] = r++;
    if(r > 100)
        r = 10;
    return 3;
}

static int powerd_start(const char* mnt_point) {
    static vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.desc, "powerd");
    dev.read = power_read;
    return device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444, false);
}

#ifdef __wasm__
int ewok_service_init(void) {
    return powerd_start("/dev/power0");
}

int ewok_service_step(void) {
    return 0;
}
#else
int main(int argc, char** argv) {
    const char* mnt_point = argc > 1 ? argv[1]: "/dev/power0";
    return powerd_start(mnt_point);
}
#endif
