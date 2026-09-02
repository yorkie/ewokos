#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/proto.h>
#include <ewoksys/core.h>
#include <ewoksys/kernel_tic.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <x/xwin.h>
#include <ewoksys/vfs.h>
#include <mouse/mouse.h>


static void input(int pid, mouse_evt_t* mevt) {
    xevent_t ev;
    bool is_scroll = mevt->button == MOUSE_BUTTON_SCROLL_UP ||
            mevt->button == MOUSE_BUTTON_SCROLL_DOWN ||
            mevt->button == MOUSE_BUTTON_SCROLL_LEFT ||
            mevt->button == MOUSE_BUTTON_SCROLL_RIGHT;

    memset(&ev, 0, sizeof(xevent_t));
    ev.type = XEVT_MOUSE;
    ev.state = mevt->state;
    if(is_scroll) {
        /*
         * Scroll events need the current cursor position so xserver can route
         * them to the window under the pointer. Do not encode them as a large
         * relative move.
         */
        ev.value.mouse.relative = 0;
        ev.value.mouse.x = mevt->x;
        ev.value.mouse.y = mevt->y;
    }
    else if(mevt->type == MOUSE_TYPE_REL){
        ev.value.mouse.relative = 1;
        ev.value.mouse.rx = mevt->x;
        ev.value.mouse.ry = mevt->y;
    }else if(mevt->type == MOUSE_TYPE_ABS){
        ev.value.mouse.relative = 0;
        ev.value.mouse.x = mevt->x;
        ev.value.mouse.y = mevt->y;
    }
    ev.value.mouse.button = mevt->button;
    proto_t in;
    PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
    dev_cntl_by_pid(pid, X_DCNTL_INPUT, &in, NULL);
    PF->clear(&in);
}

static int _xmouse_fd = -1;
static int _xmouse_pid = -1;
static int _xmouse_width;
static int _xmouse_height;
static int _xmouse_click_detect;
static uint64_t _xmouse_click_time;
static uint64_t _xmouse_drag_time;
static int _xmouse_last_x;
static int _xmouse_last_y;

static int xmouse_init(const char* dev_name) {
    _xmouse_fd = open(dev_name, O_RDONLY | O_NONBLOCK);
    if(_xmouse_fd < 0)
        return -1;
    _xmouse_pid = dev_get_pid("/dev/x");
    if(_xmouse_pid <= 0)
        return -1;

    proto_t in, out;
    PF->init(&in)->addi(&in, 0);
    PF->init(&out);
    if(dev_cntl_by_pid(_xmouse_pid, X_DCNTL_GET_INFO, &in, &out) != 0) {
        PF->clear(&in);
        PF->clear(&out);
        return -1;
    }
    xscreen_info_t scr;
    proto_read_to(&out, &scr, sizeof(scr));
    PF->clear(&in);
    PF->clear(&out);
    _xmouse_width = scr.size.w;
    _xmouse_height = scr.size.h;
    return (_xmouse_width > 0 && _xmouse_height > 0) ? 0 : -1;
}

static int xmouse_step(void) {
    mouse_evt_t mevt;
    if(mouse_read(_xmouse_fd, &mevt) != 1)
        return 0;

    if(mevt.type == MOUSE_TYPE_ABS){
        /* Browser wasm drivers already report framebuffer pixel positions;
         * hardware absolute devices retain EwokOS's 0..32767 convention. */
#ifndef __wasm__
        mevt.x = mevt.x * _xmouse_width / 32768;
        mevt.y = mevt.y * _xmouse_height / 32768;
#endif
    }

    if(mevt.type == MOUSE_TYPE_REL){
        _xmouse_last_x += mevt.x;
        _xmouse_last_y += mevt.y;
        if(_xmouse_last_x < 0) _xmouse_last_x = 0;
        if(_xmouse_last_x >= _xmouse_width) _xmouse_last_x = _xmouse_width - 1;
        if(_xmouse_last_y < 0) _xmouse_last_y = 0;
        if(_xmouse_last_y >= _xmouse_height) _xmouse_last_y = _xmouse_height - 1;
    }
    else if(mevt.type == MOUSE_TYPE_ABS){
        _xmouse_last_x = mevt.x;
        _xmouse_last_y = mevt.y;
    }

    if(mevt.button == MOUSE_BUTTON_SCROLL_UP ||
            mevt.button == MOUSE_BUTTON_SCROLL_DOWN ||
            mevt.button == MOUSE_BUTTON_SCROLL_LEFT ||
            mevt.button == MOUSE_BUTTON_SCROLL_RIGHT){
        mevt.x = _xmouse_last_x;
        mevt.y = _xmouse_last_y;
    }

    if(mevt.state == MOUSE_STATE_UP && mevt.button == MOUSE_BUTTON_LEFT){
        if(kernel_tic_ms(0) - _xmouse_click_time < 300)
            _xmouse_click_detect++;
        _xmouse_click_time = kernel_tic_ms(0);
        _xmouse_drag_time = 0;
    }
    if(mevt.state == MOUSE_STATE_DOWN && mevt.button == MOUSE_BUTTON_LEFT)
        _xmouse_drag_time = kernel_tic_ms(0);
    if(mevt.state == MOUSE_STATE_MOVE){
        _xmouse_click_detect = 0;
        _xmouse_click_time = 0;
        if(_xmouse_drag_time != 0 &&
                kernel_tic_ms(0) - _xmouse_drag_time > 100)
            mevt.state = MOUSE_STATE_DRAG;
    }
    input(_xmouse_pid, &mevt);
    return 1;
}

#ifdef __wasm__
int ewok_service_init(void) {
    return xmouse_init("/dev/mouse0");
}

int ewok_service_step(void) {
    int handled = 0;
    while(handled < 32 && xmouse_step() > 0)
        handled++;
    return 0;
}
#else
int main(int argc, char** argv) {
    const char* dev_name = argc < 2 ? "/dev/mouse0":argv[1];
    if(xmouse_init(dev_name) != 0) {
        fprintf(stderr, "xmouse error: open [%s] failed!\n", dev_name);
        return -1;
    }
    while(true) {
        xmouse_step();
        proc_usleep(3000);
    }
    close(_xmouse_fd);
    return 0;
}
#endif
