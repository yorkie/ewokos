#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

#include <ewoksys/klog.h>

int ewok_power_probe_result = -1;

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    uint8_t state[3] = {0};
    int fd = open("/dev/power0", O_RDONLY);
    if(fd < 0)
        return -1;
    int size = read(fd, state, sizeof(state));
    close(fd);
    if(size != 3 || state[2] > 100)
        return -1;
    ewok_power_probe_result = 0;
    klog("power_probe.wasm: power device online=%u charging=%u level=%u\n",
            state[0], state[1], state[2]);
    return 0;
}
