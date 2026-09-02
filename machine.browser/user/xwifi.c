#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ewoksys/kernel_tic.h>
#include <ewoksys/vdevice.h>
#include <font/font.h>
#include <graph/graph_ex.h>
#include <mouse/mouse.h>
#include <tinyjson/tinyjson.h>
#include <x/x.h>
#include <x/xwin.h>

#define WINDOW_WIDTH 620
#define WINDOW_HEIGHT 360

static x_t app_x;
static xwin_t *app_window;
static font_t *app_font;
static x_theme_t app_theme;
static char interface_name[32] = "eth0";
static char ip_address[64] = "checking...";
static char mac_address[64] = "02:45:57:4f:4b:01";
static char state_text[64] = "initializing";
static int wifi_device_available;
static uint64_t last_refresh_ms;

static void text(graph_t *g, int x, int y, const char *value, uint32_t color) {
    graph_draw_text_font(g, x, y, value, app_font,
            app_theme.fontSize, color);
}

static void button(graph_t *g, int x, int y, int w, int h,
        const char *label, uint32_t color) {
    graph_fill_round(g, x, y, w, h, 7, color);
    graph_round(g, x, y, w, h, 7, 1, 0x88ffffffu);
    graph_draw_text_font_align(g, x, y + 2, w, h - 4, label,
            app_font, app_theme.fontSize, 0xffffffffu, FONT_ALIGN_CENTER);
}

static void refresh_network(void) {
    wifi_device_available = dev_get_pid("/dev/wl0") >= 0;
    char *reply = dev_cmd("/dev/net0", "ip");
    if(reply == NULL) {
        strcpy(state_text, "network service unavailable");
        strcpy(ip_address, "not assigned");
        return;
    }
    json_var_t *array = json_parse_str(reply);
    free(reply);
    if(array == NULL) {
        strcpy(state_text, "network response invalid");
        return;
    }
    uint32_t count = json_var_array_size(array);
    if(count > 0) {
        json_var_t *item = json_var_array_get_var(array, 0);
        if(item != NULL) {
            strncpy(interface_name, json_get_str_def(item, "name", "eth0"),
                    sizeof(interface_name) - 1);
            strncpy(ip_address, json_get_str_def(item, "ip", "not assigned"),
                    sizeof(ip_address) - 1);
            strncpy(mac_address, json_get_str_def(item, "mac",
                    "02:45:57:4f:4b:01"), sizeof(mac_address) - 1);
        }
        strcpy(state_text, "connected through browser bridge");
    }
    else {
        strcpy(state_text, "browser bridge disconnected");
        strcpy(ip_address, "not assigned");
    }
    json_var_unref(array);
}

static void draw_signal(graph_t *g, int x, int y, int active) {
    for(int i = 0; i < 4; i++) {
        int h = 6 + i * 6;
        graph_fill_rect(g, x + i * 9, y + 24 - h, 6, h,
                active ? 0xff7ee787u : 0xff52606bu);
    }
}

static void repaint(xwin_t *window, graph_t *g) {
    (void)window;
    graph_clear(g, 0xff17212bu);
    graph_fill_rect(g, 0, 0, 245, g->h, 0xff202b35u);
    text(g, 18, 16, "Network adapters", 0xffffffffu);
    graph_fill_round(g, 12, 48, 221, 62, 8, 0xff315c7au);
    draw_signal(g, 28, 67, 1);
    text(g, 78, 62, "Browser network", 0xffffffffu);
    text(g, 78, 83, interface_name, 0xffc8d3dcu);

    graph_fill_round(g, 12, 124, 221, 88, 8, 0xff263440u);
    text(g, 24, 138, "Wi-Fi control", 0xffeeeeeeu);
    text(g, 24, 160, wifi_device_available ?
            "/dev/wl0 available" : "/dev/wl0 unavailable", wifi_device_available ?
            0xff7ee787u : 0xffffb86cu);
    text(g, 24, 182, wifi_device_available ?
            "scan/connect supported" : "blocked by browser sandbox", 0xffaebbc5u);
    button(g, 12, g->h - 48, 221, 34, "Refresh", 0xff315c7au);

    text(g, 270, 18, "WLAN / Network Info", 0xffffffffu);
    graph_fill_round(g, 258, 48, g->w - 276, 246, 8, 0xff202b35u);
    char line[160];
    snprintf(line, sizeof(line), "state: %s", state_text);
    text(g, 278, 68, line, 0xff7ee787u);
    snprintf(line, sizeof(line), "interface: %s", interface_name);
    text(g, 278, 98, line, 0xffd7e3eau);
    snprintf(line, sizeof(line), "ip: %s", ip_address);
    text(g, 278, 128, line, 0xffd7e3eau);
    snprintf(line, sizeof(line), "mac: %s", mac_address);
    text(g, 278, 158, line, 0xffd7e3eau);
    text(g, 278, 202, "WebAssembly owns the EwokOS network stack.",
            0xffaebbc5u);
    text(g, 278, 226, "The browser provides packet transport only.",
            0xffaebbc5u);
    text(g, 278, 250, "SSID discovery and association are not exposed",
            0xffffb86cu);
    text(g, 278, 270, "by standard browser APIs.", 0xffffb86cu);
    button(g, 258, 310, g->w - 276, 34,
            wifi_device_available ? "Scan Wi-Fi" : "Host Wi-Fi unavailable",
            wifi_device_available ? 0xff2f7d55u : 0xff52606bu);
}

static int inside(gpos_t p, int x, int y, int w, int h) {
    return p.x >= x && p.y >= y && p.x < x + w && p.y < y + h;
}

static void event(xwin_t *window, xevent_t *event_value) {
    if(event_value->type != XEVT_MOUSE || event_value->state != MOUSE_STATE_CLICK)
        return;
    gpos_t p = xwin_get_inside_pos(window,
            event_value->value.mouse.x, event_value->value.mouse.y);
    if(inside(p, 12, window->xinfo->wsr.h - 48, 221, 34)) {
        refresh_network();
        xwin_repaint(window);
    }
    else if(wifi_device_available &&
            inside(p, 258, 310, window->xinfo->wsr.w - 276, 34)) {
        char *reply = dev_cmd("/dev/wl0", "scan");
        free(reply);
        refresh_network();
        xwin_repaint(window);
    }
}

static int open_app_window(void) {
    if(app_window != NULL && app_window->fd > 0 && app_window->xinfo != NULL)
        return 0;
    if(app_window != NULL)
        xwin_destroy(app_window);
    app_x.main_win = NULL;
    app_x.terminated = false;
    app_window = xwin_open(&app_x, -1, 100, 90, WINDOW_WIDTH, WINDOW_HEIGHT,
            "xwifi.wasm", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
    if(app_window == NULL)
        return -1;
    app_window->on_repaint = repaint;
    app_window->on_event = event;
    x_set_app_name(&app_x, "/apps/xwifi/xwifi");
    xwin_set_visible(app_window, true);
    return 0;
}

void ewok_launch_argument_changed(void) {
    if(open_app_window() == 0) {
        xwin_top(app_window);
        xwin_repaint(app_window);
    }
}

int ewok_service_init(void) {
    memset(&app_x, 0, sizeof(app_x));
    x_init(&app_x, NULL);
    x_get_theme(&app_theme);
    app_font = font_new(app_theme.fontName, true);
    refresh_network();
    return app_font == NULL ? -1 : open_app_window();
}

int ewok_service_step(void) {
    if(app_window != NULL && app_window->fd > 0) {
        for(int i = 0; i < 8 && x_run_once(&app_x, NULL) == 0; i++) {}
        uint64_t now = kernel_tic_ms(0);
        if(now - last_refresh_ms >= 3000) {
            last_refresh_ms = now;
            refresh_network();
            xwin_repaint(app_window);
        }
    }
    return 0;
}
