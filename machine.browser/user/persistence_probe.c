#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <ewoksys/klog.h>

int ewok_persistence_probe_result = -1;

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    static const char path[] = "/etc/wasm-boot-count";
    char buffer[24] = {0};
    int count = 0;
    int fd = open(path, O_RDONLY);
    if(fd >= 0) {
        int size = read(fd, buffer, sizeof(buffer) - 1);
        close(fd);
        if(size > 0)
            count = atoi(buffer);
    }
    count++;
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd < 0)
        return -1;
    int size = snprintf(buffer, sizeof(buffer), "%d\n", count);
    if(write(fd, buffer, size) != size) {
        close(fd);
        return -1;
    }
    fsync(fd);
    close(fd);
    ewok_persistence_probe_result = count;
    klog("persistence_probe.wasm: ext3 persistent boot count=%d\n", count);
    return 0;
}
