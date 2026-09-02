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

#define MAX_FONTS 32
#define LIST_WIDTH 170
#define ROW_HEIGHT 28

static x_t app_x;
static xwin_t *app_window;
static font_t *ui_font;
static font_t *preview_font;
static x_theme_t app_theme;
static char font_names[MAX_FONTS][FONT_NAME_MAX];
static int font_count;
static int selected;
static int list_scroll;
static int preview_scroll;

static void load_fonts(void) {
    static const struct {
        const char *name;
        const char *path;
    } bundled[] = {
        {"system", "/usr/system/fonts/system.ttf"},
        {"smallpixel", "/usr/system/fonts/smallpixel.ttf"},
        {"Menlo", "/usr/system/fonts/Menlo.ttf"},
        {"decterm", "/usr/system/fonts/decterm.ttf"},
        {"Hack-Regular", "/usr/system/fonts/Hack-Regular.ttf"},
        {"Courier-Prime", "/usr/system/fonts/Courier-Prime.ttf"},
        {"system-cn", "/usr/system/fonts/system-cn.ttf"},
    };
    for(uint32_t i = 0; i < sizeof(bundled) / sizeof(bundled[0]); i++)
        font_load(bundled[i].name, bundled[i].path);
    proto_t out;
    PF->init(&out);
    if(dev_cntl("/dev/font", FONT_DEV_LIST, NULL, &out) == 0) {
        while(font_count < MAX_FONTS) {
            const char *name = proto_read_str(&out);
            if(name == NULL || name[0] == 0)
                break;
            strncpy(font_names[font_count], name, FONT_NAME_MAX - 1);
            font_count++;
        }
    }
    PF->clear(&out);
    if(font_count == 0) {
        strcpy(font_names[0], app_theme.fontName);
        font_count = 1;
    }
}

static void select_font(int index) {
    if(index < 0 || index >= font_count || index == selected)
        return;
    font_t *next = font_new(font_names[index], false);
    if(next == NULL)
        return;
    if(preview_font != NULL)
        font_free(preview_font);
    preview_font = next;
    selected = index;
    preview_scroll = 0;
}

static void draw_text(graph_t *g, int x, int y, const char *text,
        font_t *font, uint32_t size, uint32_t color) {
    if(font != NULL)
        graph_draw_text_font(g, x, y, text, font, size, color);
}

static void repaint(xwin_t *window, graph_t *g) {
    (void)window;
    graph_clear(g, app_theme.docBGColor);
    graph_fill_rect(g, 0, 0, LIST_WIDTH, g->h, app_theme.bgColor);
    graph_fill_rect(g, LIST_WIDTH - 1, 0, 1, g->h, 0xff52606bu);

    int visible = g->h / ROW_HEIGHT;
    if(list_scroll > font_count - visible)
        list_scroll = font_count > visible ? font_count - visible : 0;
    for(int row = 0; row < visible; row++) {
        int index = list_scroll + row;
        if(index >= font_count)
            break;
        int y = row * ROW_HEIGHT;
        uint32_t color = app_theme.fgColor;
        if(index == selected) {
            graph_fill_rect(g, 3, y + 2, LIST_WIDTH - 7,
                    ROW_HEIGHT - 4, app_theme.selectBGColor);
            color = app_theme.selectColor;
        }
        draw_text(g, 10, y + 7, font_names[index], ui_font,
                app_theme.fontSize, color);
    }

    int x = LIST_WIDTH + 18;
    int y = 12 - preview_scroll;
    draw_text(g, x, y, font_names[selected], ui_font, 14,
            app_theme.docFGColor);
    y += 30;
    static const char *samples[] = {
        "abcdefghijklmnopqrstuvwxyz",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
        "0123456789 .,+-*/=()[]{}",
        "The quick brown fox jumps over the lazy dog.",
        "EwokOS native WebAssembly font rendering",
        "中文字体演示：你好，世界！"
    };
    static const uint8_t sizes[] = {12, 16, 20, 24, 28, 32};
    for(uint32_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
        draw_text(g, x, y, samples[i], preview_font, sizes[i],
                app_theme.docFGColor);
        y += sizes[i] + 16;
    }
    graph_fill_rect(g, g->w - 8, 0, 8, g->h, 0xff34414cu);
    int content_h = y + preview_scroll;
    int thumb_h = content_h <= g->h ? g->h : g->h * g->h / content_h;
    int max_scroll = content_h > g->h ? content_h - g->h : 0;
    int thumb_y = max_scroll > 0 ? preview_scroll * (g->h - thumb_h) / max_scroll : 0;
    graph_fill_round(g, g->w - 7, thumb_y, 6, thumb_h, 3, 0xff75c7ffu);
}

static void clamp_preview_scroll(void) {
    if(preview_scroll < 0)
        preview_scroll = 0;
    if(preview_scroll > 260)
        preview_scroll = 260;
}

static void event(xwin_t *window, xevent_t *event_value) {
    if(event_value->type != XEVT_MOUSE)
        return;
    gpos_t pos = xwin_get_inside_pos(window,
            event_value->value.mouse.x, event_value->value.mouse.y);
    if(event_value->state == MOUSE_STATE_CLICK && pos.x < LIST_WIDTH) {
        select_font(list_scroll + pos.y / ROW_HEIGHT);
    }
    else if(event_value->value.mouse.button == MOUSE_BUTTON_SCROLL_UP) {
        if(pos.x < LIST_WIDTH)
            list_scroll--;
        else
            preview_scroll -= 28;
    }
    else if(event_value->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
        if(pos.x < LIST_WIDTH)
            list_scroll++;
        else
            preview_scroll += 28;
    }
    if(list_scroll < 0)
        list_scroll = 0;
    clamp_preview_scroll();
    xwin_repaint(window);
}

static int open_app_window(void) {
    if(app_window != NULL && app_window->fd > 0 && app_window->xinfo != NULL)
        return 0;
    if(app_window != NULL)
        xwin_destroy(app_window);
    app_x.main_win = NULL;
    app_x.terminated = false;
    app_window = xwin_open(&app_x, -1, 90, 70, 720, 480,
            "xfonts.wasm", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
    if(app_window == NULL)
        return -1;
    app_window->on_repaint = repaint;
    app_window->on_event = event;
    x_set_app_name(&app_x, "/apps/xfonts/xfonts");
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
    ui_font = font_new(app_theme.fontName, true);
    selected = -1;
    load_fonts();
    select_font(0);
    return ui_font == NULL || preview_font == NULL ? -1 : open_app_window();
}

int ewok_service_step(void) {
    if(app_window != NULL && app_window->fd > 0)
        for(int i = 0; i < 8 && x_run_once(&app_x, NULL) == 0; i++) {}
    return 0;
}
