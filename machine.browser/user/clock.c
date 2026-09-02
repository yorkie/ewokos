#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <font/font.h>
#include <graph/graph_ex.h>
#include <x/x.h>
#include <x/xwin.h>

static const int16_t sin60[60] = {
       0,  105,  208,  309,  407,  500,  588,  669,  743,  809,
     866,  914,  951,  978,  995, 1000,  995,  978,  951,  914,
     866,  809,  743,  669,  588,  500,  407,  309,  208,  105,
       0, -105, -208, -309, -407, -500, -588, -669, -743, -809,
    -866, -914, -951, -978, -995,-1000, -995, -978, -951, -914,
    -866, -809, -743, -669, -588, -500, -407, -309, -208, -105
};

static x_t app_x;
static xwin_t *app_window;
static font_t *app_font;
static x_theme_t app_theme;
static struct tm current_time;
static int last_second = -1;

static int isin(int tick) {
    tick %= 60;
    if(tick < 0)
        tick += 60;
    return sin60[tick];
}

static int icos(int tick) {
    return isin(tick + 15);
}

static void draw_hand(graph_t *g, int cx, int cy, int tick,
        int length, int width, uint32_t color) {
    int x = cx + icos(tick) * length / 1000;
    int y = cy + isin(tick) * length / 1000;
    graph_wline(g, cx, cy, x, y, width, color);
}

static void repaint(xwin_t *window, graph_t *g) {
    (void)window;
    graph_clear(g, 0x00000000);
    int cx = g->w / 2;
    int cy = g->h / 2;
    int radius = (g->w < g->h ? g->w : g->h) / 2 - 8;
    graph_fill_circle(g, cx, cy, radius, 0xfff7f8fa);
    graph_circle(g, cx, cy, radius, 3, 0xff263747);
    for(int tick = 0; tick < 60; tick += 5) {
        int x = cx + icos(tick - 15) * (radius - 9) / 1000;
        int y = cy + isin(tick - 15) * (radius - 9) / 1000;
        graph_fill_circle(g, x, y, 2, 0xff263747);
    }
    int second_tick = current_time.tm_sec - 15;
    int minute_tick = current_time.tm_min - 15;
    int hour_tick = ((current_time.tm_hour % 12) * 5 +
            current_time.tm_min / 12) - 15;
    draw_hand(g, cx, cy, hour_tick, radius * 45 / 100, 5, 0xff17212b);
    draw_hand(g, cx, cy, minute_tick, radius * 68 / 100, 3, 0xff17212b);
    draw_hand(g, cx, cy, second_tick, radius * 78 / 100, 2, 0xffe5534b);
    graph_fill_circle(g, cx, cy, 5, 0xffe5534b);

    char value[16];
    snprintf(value, sizeof(value), "%02d:%02d:%02d", current_time.tm_hour,
            current_time.tm_min, current_time.tm_sec);
    graph_draw_text_font_align(g, 0, g->h - 30, g->w, 20, value, app_font,
            app_theme.fontSize, 0xff263747, FONT_ALIGN_CENTER);
}

static void update_time(void) {
    time_t now = time(NULL);
    localtime_r(&now, &current_time);
}

int ewok_service_init(void) {
    x_init(&app_x, NULL);
    x_get_theme(&app_theme);
    app_font = font_new(app_theme.fontName, true);
    app_window = xwin_open(&app_x, -1, 260, 170, 220, 220,
            "Circular Clock", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
    if(app_window == NULL || app_font == NULL)
        return -1;
    app_window->on_repaint = repaint;
    x_set_app_name(&app_x, "/apps/clock/clock");
    update_time();
    last_second = current_time.tm_sec;
    xwin_set_visible(app_window, true);
    return 0;
}

int ewok_service_step(void) {
    if(app_window == NULL)
        return -1;
    for(int i = 0; i < 8 && x_run_once(&app_x, NULL) == 0; i++) {}
    if(app_x.terminated || app_window->xinfo == NULL)
        return 0;
    update_time();
    if(current_time.tm_sec != last_second) {
        last_second = current_time.tm_sec;
        xwin_repaint(app_window);
    }
    return 0;
}
