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

#define TOOLBAR_HEIGHT 36
#define STATUS_HEIGHT 24

extern int32_t wasm_host_launch_argument(char *buffer, uint32_t capacity);
extern uint32_t wasm_host_launch_generation(void);

static x_t app_x;
static xwin_t *app_window;
static font_t *app_font;
static x_theme_t app_theme;
static graph_t *image;
static int zoom_percent = 100;
static int pan_x;
static int pan_y;
static uint32_t background = 0xff202830u;
static char current_file[FS_FULL_NAME_MAX + 1];
static char status_text[128] = "No image selected";
static uint64_t last_argument_check_ms;
static uint32_t launch_generation;

static void draw_button(graph_t *g, int x, int width, const char *label) {
    graph_fill_round(g, x, 5, width, 26, 7, 0xff315c7au);
    graph_round(g, x, 5, width, 26, 7, 1, 0x88ffffffu);
    graph_draw_text_font_align(g, x, 6, width, 24, label, app_font,
            app_theme.fontSize, 0xffffffffu, FONT_ALIGN_CENTER);
}

static void load_image(const char *path) {
    graph_t *loaded = graph_image_new(path);
    if(loaded == NULL) {
        snprintf(status_text, sizeof(status_text), "Cannot decode %s", path);
        return;
    }
    if(image != NULL)
        graph_free(image);
    image = loaded;
    zoom_percent = 100;
    pan_x = 0;
    pan_y = 0;
    memset(current_file, 0, sizeof(current_file));
    strncpy(current_file, path, sizeof(current_file) - 1);
    snprintf(status_text, sizeof(status_text), "%s · %dx%d",
            current_file, image->w, image->h);
}

static int refresh_launch_argument(void) {
    uint32_t generation = wasm_host_launch_generation();
    if(generation == launch_generation)
        return 0;
    launch_generation = generation;
    char value[FS_FULL_NAME_MAX + 1];
    memset(value, 0, sizeof(value));
    wasm_host_launch_argument(value, sizeof(value));
    if(value[0] == 0)
        return 0;
    load_image(value);
    return 1;
}

static void fit_image(int width, int height) {
    if(image == NULL)
        return;
    int view_height = height - TOOLBAR_HEIGHT - STATUS_HEIGHT;
    int zx = width * 100 / image->w;
    int zy = view_height * 100 / image->h;
    zoom_percent = zx < zy ? zx : zy;
    if(zoom_percent < 10)
        zoom_percent = 10;
    if(zoom_percent > 400)
        zoom_percent = 400;
    pan_x = 0;
    pan_y = 0;
}

static void repaint(xwin_t *window, graph_t *g) {
    (void)window;
    graph_clear(g, background);
    graph_fill_rect(g, 0, 0, g->w, TOOLBAR_HEIGHT, 0xff263440u);
    draw_button(g, 8, 42, "+");
    draw_button(g, 56, 42, "-");
    draw_button(g, 104, 52, "Fit");
    draw_button(g, 162, 52, "BG");
    graph_draw_text_font(g, 226, 10,
            current_file[0] != 0 ? current_file : "ximg.wasm",
            app_font, app_theme.fontSize, 0xffeeeeeeu);

    int view_y = TOOLBAR_HEIGHT;
    int view_height = g->h - TOOLBAR_HEIGHT - STATUS_HEIGHT;
    graph_fill_rect(g, 0, view_y, g->w, view_height, background);
    if(image != NULL) {
        int width = image->w * zoom_percent / 100;
        int height = image->h * zoom_percent / 100;
        if(width < 1)
            width = 1;
        if(height < 1)
            height = 1;
        int x = (g->w - width) / 2 + pan_x;
        int y = view_y + (view_height - height) / 2 + pan_y;
        graph_blt_fit_alpha(image, 0, 0, image->w, image->h,
                g, x, y, width, height, 0xff);
    }

    graph_fill_rect(g, 0, g->h - STATUS_HEIGHT, g->w,
            STATUS_HEIGHT, 0xff263440u);
    char footer[160];
    snprintf(footer, sizeof(footer), "%s · zoom %d%% · offset %d,%d",
            status_text, zoom_percent, pan_x, pan_y);
    graph_draw_text_font(g, 8, g->h - STATUS_HEIGHT + 5, footer,
            app_font, app_theme.fontSize, 0xffd7e3eau);
}

static void change_zoom(int delta) {
    zoom_percent += delta;
    if(zoom_percent < 10)
        zoom_percent = 10;
    if(zoom_percent > 400)
        zoom_percent = 400;
}

static void event(xwin_t *window, xevent_t *event_value) {
    if(event_value->type == XEVT_MOUSE) {
        if(event_value->state == MOUSE_STATE_CLICK) {
            gpos_t pos = xwin_get_inside_pos(window,
                    event_value->value.mouse.x, event_value->value.mouse.y);
            if(pos.y < TOOLBAR_HEIGHT && pos.x >= 8 && pos.x < 50)
                change_zoom(20);
            else if(pos.y < TOOLBAR_HEIGHT && pos.x >= 56 && pos.x < 98)
                change_zoom(-20);
            else if(pos.y < TOOLBAR_HEIGHT && pos.x >= 104 && pos.x < 156)
                fit_image(window->xinfo->wsr.w, window->xinfo->wsr.h);
            else if(pos.y < TOOLBAR_HEIGHT && pos.x >= 162 && pos.x < 214) {
                static const uint32_t colors[] = {
                    0xff202830u, 0xffeeeeeeu, 0xff555588u, 0xff111111u,
                };
                for(uint32_t i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
                    if(background == colors[i]) {
                        background = colors[(i + 1) %
                                (sizeof(colors) / sizeof(colors[0]))];
                        break;
                    }
                }
            }
        }
        else if(event_value->value.mouse.button == MOUSE_BUTTON_SCROLL_UP)
            change_zoom(10);
        else if(event_value->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN)
            change_zoom(-10);
        xwin_repaint(window);
    }
    else if(event_value->type == XEVT_IM &&
            event_value->state == XIM_STATE_PRESS) {
        if(event_value->value.im.value == KEY_LEFT)
            pan_x += 16;
        else if(event_value->value.im.value == KEY_RIGHT)
            pan_x -= 16;
        else if(event_value->value.im.value == KEY_UP)
            pan_y += 16;
        else if(event_value->value.im.value == KEY_DOWN)
            pan_y -= 16;
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
    app_window = xwin_open(&app_x, -1, 100, 70, 720, 520,
            "ximg.wasm", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
    if(app_window == NULL)
        return -1;
    if(image != NULL)
        fit_image(app_window->xinfo->wsr.w, app_window->xinfo->wsr.h);
    app_window->on_repaint = repaint;
    app_window->on_event = event;
    x_set_app_name(&app_x, "/apps/ximg/ximg");
    xwin_set_visible(app_window, true);
    xwin_call_xim(app_window, true);
    return 0;
}

void ewok_launch_argument_changed(void) {
    if(!refresh_launch_argument() || open_app_window() != 0)
        return;
    xwin_top(app_window);
    xwin_repaint(app_window);
}

int ewok_service_init(void) {
    memset(&app_x, 0, sizeof(app_x));
    x_init(&app_x, NULL);
    x_get_theme(&app_theme);
    app_font = font_new(app_theme.fontName, true);
    refresh_launch_argument();
    if(app_font == NULL || open_app_window() != 0)
        return -1;
    return 0;
}

int ewok_service_step(void) {
    if(app_window == NULL)
        return -1;
    if(app_window->fd > 0)
        for(int i = 0; i < 8 && x_run_once(&app_x, NULL) == 0; i++) {}
    uint64_t now = kernel_tic_ms(0);
    if(now - last_argument_check_ms >= 250) {
        last_argument_check_ms = now;
        ewok_launch_argument_changed();
    }
    return 0;
}
