#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ewoksys/kernel_tic.h>
#include <ewoksys/keydef.h>
#include <font/font.h>
#include <graph/graph_ex.h>
#include <graph/graph_image.h>
#include <mouse/mouse.h>
#include <x/x.h>
#include <x/xwin.h>

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 420
#define LIST_TOP 174
#define LIST_ROW 25

static x_t app_x;
static xwin_t *app_window;
static font_t *app_font;
static graph_t *walk_image;
static x_theme_t app_theme;
static char edit_text[96] = "hello!";
static int edit_length = 6;
static int selected_left;
static int selected_right;
static int animation_frame;
static int animation_x;
static int dialog_visible;
static char dialog_result[32] = "Dialog Test";
static uint64_t last_animation_ms;

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

static void draw_list(graph_t *g, int x, int y, int w, int h, int selected) {
    graph_fill_rect(g, x, y, w, h, 0xffd7dce0u);
    graph_round(g, x, y, w, h, 4, 1, 0xff52606bu);
    int rows = h / LIST_ROW;
    for(int i = 0; i < rows; i++) {
        int ry = y + i * LIST_ROW;
        if(i == selected)
            graph_fill_rect(g, x + 1, ry + 1, w - 2, LIST_ROW - 2,
                    0xffe5534bu);
        char number[16];
        snprintf(number, sizeof(number), "Item %02d", i);
        text(g, x + 8, ry + 6, number,
                i == selected ? 0xffffffffu : 0xff17212bu);
    }
}

static void repaint(xwin_t *window, graph_t *g) {
    (void)window;
    graph_clear(g, 0xffedf1f4u);
    graph_fill_round(g, 14, 12, g->w - 28, 36, 7, 0xffffffffu);
    graph_round(g, 14, 12, g->w - 28, 36, 7, 1, 0xff75c7ffu);
    text(g, 26, 23, edit_text, 0xff17212bu);
    graph_fill_rect(g, 26 + edit_length * 8, 20, 1, 19, 0xff17212bu);

    graph_gradation(g, 14, 60, g->w - 28, 88,
            0xffdbeafeu, 0xff86efacu, true);
    if(walk_image != NULL && walk_image->w >= 8) {
        int fw = walk_image->w / 8;
        graph_blt_alpha(walk_image, animation_frame * fw, 0,
                fw, walk_image->h, g, 20 + animation_x,
                60 + (88 - walk_image->h) / 2, fw, walk_image->h, 0xff);
    }
    text(g, 140, 92, edit_text, 0xff17212bu);

    button(g, 14, 156, 150, 32, dialog_result, 0xff315c7au);
    button(g, 174, 156, 110, 32, "disabled", 0xff89939cu);
    text(g, 310, 165, "C/Widget behavior recreated in native Wasm",
            0xff52606bu);
    draw_list(g, 14, 198, 270, g->h - 212, selected_left);
    draw_list(g, 300, 198, g->w - 314, g->h - 212, selected_right);

    if(dialog_visible) {
        graph_fill_rect(g, 0, 0, g->w, g->h, 0x66000000u);
        int dx = (g->w - 300) / 2;
        int dy = (g->h - 150) / 2;
        graph_fill_round(g, dx, dy, 300, 150, 10, 0xff202b35u);
        graph_round(g, dx, dy, 300, 150, 10, 2, 0xff75c7ffu);
        graph_draw_text_font_align(g, dx, dy + 25, 300, 28,
                "Dialog Test", app_font, 16, 0xffffffffu,
                FONT_ALIGN_CENTER);
        button(g, dx + 28, dy + 92, 108, 34, "Confirm", 0xff2f7d55u);
        button(g, dx + 164, dy + 92, 108, 34, "Cancel", 0xff7a3f48u);
    }
}

static int inside(gpos_t p, int x, int y, int w, int h) {
    return p.x >= x && p.y >= y && p.x < x + w && p.y < y + h;
}

static void event(xwin_t *window, xevent_t *event_value) {
    if(event_value->type == XEVT_IM && event_value->state == XIM_STATE_PRESS) {
        int key = event_value->value.im.value;
        if(key == KEY_BACKSPACE && edit_length > 0)
            edit_text[--edit_length] = 0;
        else if(key >= 32 && key < 127 && edit_length < (int)sizeof(edit_text) - 1) {
            edit_text[edit_length++] = (char)key;
            edit_text[edit_length] = 0;
        }
        xwin_repaint(window);
        return;
    }
    if(event_value->type != XEVT_MOUSE || event_value->state != MOUSE_STATE_CLICK)
        return;
    gpos_t p = xwin_get_inside_pos(window,
            event_value->value.mouse.x, event_value->value.mouse.y);
    if(dialog_visible) {
        int dx = (window->xinfo->wsr.w - 300) / 2;
        int dy = (window->xinfo->wsr.h - 150) / 2;
        if(inside(p, dx + 28, dy + 92, 108, 34)) {
            strcpy(dialog_result, "Confirmed");
            dialog_visible = 0;
        }
        else if(inside(p, dx + 164, dy + 92, 108, 34)) {
            strcpy(dialog_result, "Canceled");
            dialog_visible = 0;
        }
    }
    else if(inside(p, 14, 12, window->xinfo->wsr.w - 28, 36))
        xwin_call_xim(window, true);
    else if(inside(p, 14, 156, 150, 32))
        dialog_visible = 1;
    else if(p.y >= 198 && p.x >= 14 && p.x < 284)
        selected_left = (p.y - 198) / LIST_ROW;
    else if(p.y >= 198 && p.x >= 300)
        selected_right = (p.y - 198) / LIST_ROW;
    xwin_repaint(window);
}

static int open_app_window(void) {
    if(app_window != NULL && app_window->fd > 0 && app_window->xinfo != NULL)
        return 0;
    if(app_window != NULL)
        xwin_destroy(app_window);
    app_x.main_win = NULL;
    app_x.terminated = false;
    app_window = xwin_open(&app_x, -1, 110, 70, WINDOW_WIDTH, WINDOW_HEIGHT,
            "wDemo.wasm", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
    if(app_window == NULL)
        return -1;
    app_window->on_repaint = repaint;
    app_window->on_event = event;
    x_set_app_name(&app_x, "/apps/wDemo/wDemo");
    xwin_set_visible(app_window, true);
    xwin_call_xim(app_window, true);
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
    walk_image = graph_image_new("/apps/wDemo/res/data/walk.png");
    return app_font == NULL ? -1 : open_app_window();
}

int ewok_service_step(void) {
    if(app_window != NULL && app_window->fd > 0) {
        for(int i = 0; i < 8 && x_run_once(&app_x, NULL) == 0; i++) {}
        uint64_t now = kernel_tic_ms(0);
        if(now - last_animation_ms >= 80) {
            last_animation_ms = now;
            animation_frame = (animation_frame + 1) % 8;
            animation_x = (animation_x + 3) % 480;
            xwin_repaint(app_window);
        }
    }
    return 0;
}
