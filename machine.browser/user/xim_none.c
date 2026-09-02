#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <ewoksys/keydef.h>
#include <ewoksys/klog.h>
#include <ewoksys/vdevice.h>
#include <keyb/keyb.h>
#include <x/xwin.h>

static int keyb_fd = -1;
static int x_pid = -1;
static uint8_t shift;
static uint8_t ctrl;
static uint32_t forwarded;

static void forward_key(uint8_t key, uint8_t state) {
    xevent_t event;
    memset(&event, 0, sizeof(event));
    event.type = XEVT_IM;
    event.state = state;
    event.value.im.key_code = key;

    if(state == KEYB_STATE_PRESS) {
        if(key == KEY_LSHIFT || key == KEY_RSHIFT)
            shift = key;
        else if(key == KEY_CTRL)
            ctrl = 1;
        event.value.im.shift = shift;
        event.value.im.ctrl = ctrl;
        event.value.im.value = shift ? keyb_shift_value(key) :
                (ctrl ? keyb_ctrl_value(key) : key);
    }
    else {
        if(key == KEY_LSHIFT || key == KEY_RSHIFT)
            shift = 0;
        else if(key == KEY_CTRL)
            ctrl = 0;
        event.value.im.value = key;
    }

    proto_t in;
    PF->init(&in)->add(&in, &event, sizeof(event));
    if(dev_cntl_by_pid(x_pid, X_DCNTL_INPUT, &in, NULL) != 0)
        x_pid = -1;
    else if(forwarded++ == 0)
        klog("xim.wasm: browser keyboard event reached xserver\n");
    PF->clear(&in);
}

int ewok_service_init(void) {
    keyb_fd = open("/dev/keyb0", O_RDONLY | O_NONBLOCK);
    x_pid = dev_get_pid("/dev/x");
    return keyb_fd < 0 || x_pid <= 0 ? -1 : 0;
}

int ewok_service_step(void) {
    if(x_pid <= 0)
        x_pid = dev_get_pid("/dev/x");
    if(x_pid <= 0 || keyb_fd < 0)
        return 0;

    keyb_evt_t events[KEYB_EVT_MAX];
    int count = keyb_read(keyb_fd, events, KEYB_EVT_MAX);
    for(int i = 0; i < count; i++)
        forward_key(events[i].key, events[i].state);
    return 0;
}
