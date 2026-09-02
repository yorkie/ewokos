#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ewoksys/proto.h>
#include <ewoksys/vdevice.h>
#include <font/font.h>
#include <graph/graph_ex.h>
#include <mouse/mouse.h>
#include <x/x.h>
#include <x/xwin.h>

#define WINDOW_WIDTH 440
#define WINDOW_HEIGHT 410

static x_t app_x;
static xwin_t *app_window;
static font_t *app_font;
static x_theme_t loaded_theme;
static x_theme_t edited_theme;
static int apply_result;

static const uint32_t palette[] = {
    0xff17212bu, 0xff202b35u, 0xff315c7au, 0xff58606eu,
    0xfff1f3f5u, 0xffffffffu, 0xff7ee787u, 0xff75c7ffu,
    0xffe5534bu, 0xff8b5cf6u
};
static const char *fonts[] = {
    "system", "system-cn", "Menlo", "Hack-Regular", "Courier-Prime", "decterm"
};

static uint32_t next_color(uint32_t value) {
    uint32_t count = sizeof(palette) / sizeof(palette[0]);
    for(uint32_t i = 0; i < count; i++)
        if(palette[i] == value)
            return palette[(i + 1) % count];
    return palette[0];
}

static void next_font(void) {
    uint32_t count = sizeof(fonts) / sizeof(fonts[0]);
    for(uint32_t i = 0; i < count; i++) {
        if(strcmp(edited_theme.fontName, fonts[i]) == 0) {
            strncpy(edited_theme.fontName, fonts[(i + 1) % count],
                    sizeof(edited_theme.fontName) - 1);
            return;
        }
    }
    strncpy(edited_theme.fontName, fonts[0], sizeof(edited_theme.fontName) - 1);
}

static void text(graph_t *g, int x, int y, const char *value, uint32_t color) {
    if(app_font != NULL)
        graph_draw_text_font(g, x, y, value, app_font, 14, color);
}

static void button(graph_t *g, int x, int y, int w, int h,
        const char *label, uint32_t color) {
    graph_fill_round(g, x, y, w, h, 7, color);
    graph_round(g, x, y, w, h, 7, 1, 0x88ffffffu);
    graph_draw_text_font_align(g, x, y + 2, w, h - 4, label,
            app_font, 14, 0xffffffffu, FONT_ALIGN_CENTER);
}

static void color_row(graph_t *g, int y, const char *label, uint32_t color) {
    graph_fill_round(g, 20, y, 400, 32, 7, 0xff263440u);
    text(g, 32, y + 8, label, 0xffeeeeeeu);
    graph_fill_round(g, 316, y + 5, 90, 22, 6, color);
    graph_round(g, 316, y + 5, 90, 22, 6, 1, 0xaaffffffu);
}

static void repaint(xwin_t *window, graph_t *g) {
    (void)window;
    graph_clear(g, 0xff17212bu);
    text(g, 20, 12, "Application Theme", 0xffffffffu);
    graph_fill_round(g, 20, 38, 400, 54, 9, edited_theme.titleBGColor);
    text(g, 38, 58, "EwokOS theme preview", edited_theme.titleColor);
    color_row(g, 104, "Window background", edited_theme.bgColor);
    color_row(g, 140, "Window foreground", edited_theme.fgColor);
    color_row(g, 176, "Document background", edited_theme.docBGColor);
    color_row(g, 212, "Document foreground", edited_theme.docFGColor);
    color_row(g, 248, "Selection background", edited_theme.selectBGColor);
    color_row(g, 284, "Title background", edited_theme.titleBGColor);
    graph_fill_round(g, 20, 320, 400, 32, 7, 0xff263440u);
    text(g, 32, 328, "Font", 0xffeeeeeeu);
    button(g, 238, 325, 168, 22, edited_theme.fontName, 0xff315c7au);
    button(g, 20, 366, 120, 30, "Apply", 0xff2f7d55u);
    button(g, 160, 366, 120, 30, "Reset", 0xff315c7au);
    button(g, 300, 366, 120, 30, "Close", 0xff7a3f48u);
    if(apply_result != 0)
        text(g, 302, 12, "Apply failed", 0xffff7777u);
}

static int load_theme(void) {
    proto_t out;
    PF->init(&out);
    int result = dev_cntl("/dev/x", X_DCNTL_GET_THEME, NULL, &out);
    if(result == 0)
        proto_read_to(&out, &loaded_theme, sizeof(loaded_theme));
    PF->clear(&out);
    edited_theme = loaded_theme;
    return result;
}

static int apply_theme(void) {
    edited_theme.uuid++;
    proto_t in;
    PF->init(&in)->add(&in, &edited_theme, sizeof(edited_theme));
    int result = dev_cntl("/dev/x", X_DCNTL_SET_THEME, &in, NULL);
    PF->clear(&in);
    if(result == 0)
        loaded_theme = edited_theme;
    return result;
}

static int inside(gpos_t p, int x, int y, int w, int h) {
    return p.x >= x && p.y >= y && p.x < x + w && p.y < y + h;
}

static void event(xwin_t *window, xevent_t *event_value) {
    if(event_value->type != XEVT_MOUSE || event_value->state != MOUSE_STATE_CLICK)
        return;
    gpos_t p = xwin_get_inside_pos(window,
            event_value->value.mouse.x, event_value->value.mouse.y);
    uint32_t *colors[] = {
        &edited_theme.bgColor, &edited_theme.fgColor,
        &edited_theme.docBGColor, &edited_theme.docFGColor,
        &edited_theme.selectBGColor, &edited_theme.titleBGColor
    };
    for(int i = 0; i < 6; i++) {
        if(inside(p, 316, 109 + i * 36, 90, 22)) {
            *colors[i] = next_color(*colors[i]);
            xwin_repaint(window);
            return;
        }
    }
    if(inside(p, 238, 325, 168, 22))
        next_font();
    else if(inside(p, 20, 366, 120, 30))
        apply_result = apply_theme();
    else if(inside(p, 160, 366, 120, 30)) {
        edited_theme = loaded_theme;
        apply_result = 0;
    }
    else if(inside(p, 300, 366, 120, 30)) {
        xwin_close(app_window);
        app_window = NULL;
        return;
    }
    xwin_repaint(window);
}

static int open_app_window(void) {
    if(app_window != NULL && app_window->fd > 0 && app_window->xinfo != NULL)
        return 0;
    if(app_window != NULL)
        xwin_destroy(app_window);
    app_x.main_win = NULL;
    app_x.terminated = false;
    app_window = xwin_open(&app_x, -1, 160, 90, WINDOW_WIDTH, WINDOW_HEIGHT,
            "xtheme.wasm", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
    if(app_window == NULL)
        return -1;
    app_window->on_repaint = repaint;
    app_window->on_event = event;
    x_set_app_name(&app_x, "/apps/xtheme/xtheme");
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
    if(load_theme() != 0)
        return -1;
    app_font = font_new(loaded_theme.fontName, true);
    return app_font == NULL ? -1 : open_app_window();
}

int ewok_service_step(void) {
    if(app_window != NULL && app_window->fd > 0)
        for(int i = 0; i < 8 && x_run_once(&app_x, NULL) == 0; i++) {}
    return 0;
}
