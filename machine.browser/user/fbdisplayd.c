#include <stdint.h>
#include <string.h>
#include <displayd/displayd.h>

extern int32_t wasm_host_framebuffer_configure(uint32_t width,
        uint32_t height);
extern int32_t wasm_host_framebuffer_flush(const uint32_t *pixels,
        uint32_t width, uint32_t height);

static disp_info_t browser_fbinfo;
static fbdisplayd_t browser_display;

static int32_t browser_init(uint32_t width, uint32_t height, uint32_t depth) {
    if(depth != 32 || width == 0 || height == 0)
        return -1;
    memset(&browser_fbinfo, 0, sizeof(browser_fbinfo));
    browser_fbinfo.width = width;
    browser_fbinfo.height = height;
    browser_fbinfo.depth = 32;
    browser_fbinfo.pitch = width * 4;
    browser_fbinfo.size = width * height * 4;
    return wasm_host_framebuffer_configure(width, height);
}

static disp_info_t *browser_get_info(void) {
    return &browser_fbinfo;
}

static uint32_t browser_flush(const disp_info_t *info, const graph_t *graph) {
    (void)info;
    if(graph == NULL || graph->buffer == NULL)
        return 0;
    if(wasm_host_framebuffer_flush(graph->buffer, graph->w, graph->h) != 0)
        return 0;
    return (uint32_t)graph->w * (uint32_t)graph->h * 4;
}

static void browser_splash(graph_t *graph, const char *logo) {
    (void)logo;
    for(int32_t y = 0; y < graph->h; y++) {
        uint32_t green = 0x18u + (uint32_t)y * 0x28u /
            (uint32_t)(graph->h > 1 ? graph->h - 1 : 1);
        uint32_t color = 0xff000000u | (green << 8) | 0x10u;
        for(int32_t x = 0; x < graph->w; x++)
            graph->buffer[y * graph->w + x] = color;
    }
}

int ewok_service_init(void) {
    memset(&browser_display, 0, sizeof(browser_display));
    browser_display.flush = browser_flush;
    browser_display.init = browser_init;
    browser_display.get_info = browser_get_info;
    browser_display.splash = browser_splash;
    return fbdisplayd_wasm_start(&browser_display, "/dev/disp0", 640, 480,
            "", 0);
}

int ewok_service_step(void) { return 0; }

int32_t ewok_display_resize(uint32_t width, uint32_t height) {
    if(width < 320 || height < 240 || width > 2560 || height > 1600)
        return -1;
    return fbdisplayd_resize(width, height, 32);
}
