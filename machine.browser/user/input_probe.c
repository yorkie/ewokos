#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <ewoksys/klog.h>

static int key_fd = -1;
static int mouse_fd = -1;
static uint32_t key_events;
static uint32_t mouse_events;

uint32_t ewok_input_probe_keys(void) { return key_events; }
uint32_t ewok_input_probe_mouse(void) { return mouse_events; }

int ewok_service_init(void) {
    key_fd = open("/dev/keyb0", O_RDONLY | O_NONBLOCK);
    mouse_fd = open("/dev/mouse0", O_RDONLY | O_NONBLOCK);
    return key_fd < 0 || mouse_fd < 0 ? -1 : 0;
}

int ewok_service_step(void) {
    uint8_t keys[6];
    uint8_t mouse[8];
    int32_t count = read(key_fd, keys, sizeof(keys));
    if(count > 0) {
        key_events += (uint32_t)count;
        klog("input.wasm: consumed %d keyboard byte(s)\n", count);
    }
    count = read(mouse_fd, mouse, sizeof(mouse));
    if(count == (int32_t)sizeof(mouse)) {
        mouse_events++;
        klog("input.wasm: consumed browser pointer event %d\n", mouse_events);
    }
    return 0;
}
