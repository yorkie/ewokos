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

#define WINDOW_WIDTH 420
#define WINDOW_HEIGHT 360
#define ROW_X 24
#define ROW_WIDTH 372
#define ROW_HEIGHT 34

static x_t app_x;
static xwin_t *app_window;
static font_t *app_font;
static x_theme_t app_theme;
static xwm_theme_t original_theme;
static xwm_theme_t edited_theme;
static int apply_result;

static const uint32_t palette[] = {
    0xff202b35u, 0xff315c7au, 0xff555588u, 0xff2f6f73u,
    0xff705060u, 0xffaaaaaau, 0xffddddddU, 0xff222222u,
};

static uint32_t next_color(uint32_t color) {
    uint32_t count = sizeof(palette) / sizeof(palette[0]);
    for(uint32_t i = 0; i < count; i++) {
        if(palette[i] == color)
            return palette[(i + 1) % count];
    }
    return palette[0];
}

static void draw_text(graph_t *g, int x, int y, const char *text,
        uint32_t color) {
    if(app_font != NULL)
        graph_draw_text_font(g, x, y, text, app_font,
                app_theme.fontSize, color);
}

static void draw_button(graph_t *g, int x, int y, int w, int h,
        const char *label, uint32_t color) {
    graph_fill_round(g, x, y, w, h, 8, color);
    graph_round(g, x, y, w, h, 8, 1, 0x88ffffffu);
    if(app_font != NULL)
        graph_draw_text_font_align(g, x, y + 2, w, h - 4, label,
                app_font, app_theme.fontSize, 0xffffffffu,
                FONT_ALIGN_CENTER);
}

static void draw_color_row(graph_t *g, int y, const char *label,
        uint32_t color) {
    graph_fill_round(g, ROW_X, y, ROW_WIDTH, ROW_HEIGHT, 8, 0xff263440u);
    draw_text(g, ROW_X + 12, y + 9, label, 0xffeeeeeeu);
    graph_fill_round(g, 300, y + 5, 82, ROW_HEIGHT - 10, 7, color);
    graph_round(g, 300, y + 5, 82, ROW_HEIGHT - 10, 7, 1,
            0xccffffffu);
}

static void repaint(xwin_t *window, graph_t *g) {
    (void)window;
    graph_clear(g, 0xff17212bu);
    draw_text(g, ROW_X, 12, "Window Manager Theme", 0xffffffffu);

    graph_fill_round(g, ROW_X, 38, ROW_WIDTH, 72, 10,
            edited_theme.desktopBGColor);
    graph_fill_round(g, 75, 55, 270, 42,
            edited_theme.round, edited_theme.frameBGColor);
    graph_round(g, 75, 55, 270, 42, edited_theme.round,
            edited_theme.frameW > 0 ? edited_theme.frameW : 1,
            edited_theme.frameFGColor);
    draw_text(g, 92, 69, "EwokOS theme preview",
            edited_theme.frameFGColor);

    draw_color_row(g, 122, "Desktop color", edited_theme.desktopBGColor);
    draw_color_row(g, 162, "Frame color", edited_theme.frameBGColor);
    draw_color_row(g, 202, "Frame text", edited_theme.frameFGColor);

    graph_fill_round(g, ROW_X, 242, ROW_WIDTH, ROW_HEIGHT, 8, 0xff263440u);
    draw_text(g, ROW_X + 12, 251, "Corner radius", 0xffeeeeeeu);
    draw_button(g, 286, 247, 30, 24, "-", 0xff475866u);
    char radius[16];
    snprintf(radius, sizeof(radius), "%u", edited_theme.round);
    if(app_font != NULL)
        graph_draw_text_font_align(g, 317, 249, 34, 20, radius,
                app_font, app_theme.fontSize, 0xffffffffu,
                FONT_ALIGN_CENTER);
    draw_button(g, 352, 247, 30, 24, "+", 0xff475866u);

    graph_fill_round(g, ROW_X, 282, ROW_WIDTH, 24, 7, 0xff263440u);
    draw_text(g, ROW_X + 12, 287, "Wallpaper", 0xffeeeeeeu);
    draw_button(g, 300, 284, 82, 20,
            edited_theme.patternName[0] == 0 ? "Off" : "On",
            edited_theme.patternName[0] == 0 ? 0xff555555u : 0xff2f7d55u);

    draw_button(g, 24, 318, 112, 30, "Apply", 0xff2f7d55u);
    draw_button(g, 154, 318, 112, 30, "Reset", 0xff315c7au);
    draw_button(g, 284, 318, 112, 30, "Close", 0xff7a3f48u);
    if(apply_result != 0)
        draw_text(g, 286, 12, "Apply failed", 0xffff7777u);
}

static int load_theme(xwm_theme_t *theme) {
    proto_t out;
    PF->init(&out);
    int result = dev_cntl("/dev/x", X_DCNTL_GET_XWM_THEME, NULL, &out);
    if(result == 0)
        proto_read_to(&out, theme, sizeof(*theme));
    PF->clear(&out);
    return result;
}

static int apply_theme(void) {
    proto_t in;
    PF->init(&in)->add(&in, &edited_theme, sizeof(edited_theme));
    int result = dev_cntl("/dev/x", X_DCNTL_SET_XWM_THEME, &in, NULL);
    PF->clear(&in);
    return result;
}

static int in_rect(gpos_t pos, int x, int y, int w, int h) {
    return pos.x >= x && pos.y >= y && pos.x < x + w && pos.y < y + h;
}

static void event(xwin_t *window, xevent_t *event_value) {
    if(event_value->type != XEVT_MOUSE ||
            event_value->state != MOUSE_STATE_CLICK)
        return;
    gpos_t pos = xwin_get_inside_pos(window,
            event_value->value.mouse.x, event_value->value.mouse.y);

    if(in_rect(pos, 300, 127, 82, 24))
        edited_theme.desktopBGColor = next_color(edited_theme.desktopBGColor);
    else if(in_rect(pos, 300, 167, 82, 24))
        edited_theme.frameBGColor = next_color(edited_theme.frameBGColor);
    else if(in_rect(pos, 300, 207, 82, 24))
        edited_theme.frameFGColor = next_color(edited_theme.frameFGColor);
    else if(in_rect(pos, 286, 247, 30, 24) && edited_theme.round > 0)
        edited_theme.round--;
    else if(in_rect(pos, 352, 247, 30, 24) && edited_theme.round < 32)
        edited_theme.round++;
    else if(in_rect(pos, 300, 284, 82, 20)) {
        if(edited_theme.patternName[0] == 0)
            strncpy(edited_theme.patternName,
                    "/usr/system/images/wallpapers/wallpaper1.png",
                    sizeof(edited_theme.patternName) - 1);
        else
            edited_theme.patternName[0] = 0;
    }
    else if(in_rect(pos, 24, 318, 112, 30)) {
        apply_result = apply_theme();
        if(apply_result == 0)
            original_theme = edited_theme;
    }
    else if(in_rect(pos, 154, 318, 112, 30)) {
        edited_theme = original_theme;
        apply_result = 0;
    }
    else if(in_rect(pos, 284, 318, 112, 30)) {
        xwin_close(app_window);
        app_window = NULL;
        return;
    }
    if(app_window != NULL)
        xwin_repaint(app_window);
}

int ewok_service_init(void) {
    memset(&app_x, 0, sizeof(app_x));
    x_init(&app_x, NULL);
    x_get_theme(&app_theme);
    app_font = font_new(app_theme.fontName, true);
    if(load_theme(&original_theme) != 0)
        return -1;
    edited_theme = original_theme;
    app_window = xwin_open(&app_x, -1, -1, -1,
            WINDOW_WIDTH, WINDOW_HEIGHT, "theme.wasm",
            XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
    if(app_window == NULL || app_font == NULL)
        return -1;
    app_window->on_repaint = repaint;
    app_window->on_event = event;
    x_set_app_name(&app_x, "/apps/xwm_theme/xwm_theme");
    xwin_set_visible(app_window, true);
    return 0;
}

int ewok_service_step(void) {
    if(app_window == NULL)
        return 0;
    for(int i = 0; i < 8 && x_run_once(&app_x, NULL) == 0; i++) {}
    return 0;
}
