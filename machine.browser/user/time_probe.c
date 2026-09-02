#include <stdint.h>
#include <time.h>

#include <ewoksys/klog.h>

int ewok_time_probe_result = -1;

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    time_t now = time(NULL);
    if(now < (time_t)1700000000)
        return -1;
    ewok_time_probe_result = 0;
    klog("time_probe.wasm: EwokOS realtime is %u\n", (uint32_t)now);
    return 0;
}
