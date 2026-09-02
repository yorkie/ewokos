#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include <font/font.h>
#include <graph/graph_ex.h>
#include <graph/graph_image.h>
#include <mouse/mouse.h>
#include <x/x.h>
#include <x/xwin.h>

#define DOCK_ITEMS 5
#define DOCK_ITEM_SIZE 64
#define DOCK_ICON_SIZE 38
#define STATUS_HEIGHT 20

typedef struct {
    const char *name;
    const char *program;
    const char *icon_path;
    graph_t *icon;
} dock_item_t;

static dock_item_t dock_items[DOCK_ITEMS] = {
    { "xfinder", "/apps/xfinder/xfinder", "/apps/xfinder/res/icon.png", NULL },
    { "xapps", "/apps/xapps/xapps", "/apps/xapps/res/icon.png", NULL },
    { "xterm", "/apps/xterm/xterm", "/apps/xterm/res/icon.png", NULL },
    { "xcores", "/apps/xcores/xcores", "/apps/xcores/res/icon.png", NULL },
    { "theme", "/apps/xwm_theme/xwm_theme", "/apps/xwm_theme/res/icon.png", NULL },
};

static x_t desktop_x;
static xwin_t *dock_window;
static xwin_t *status_window;
static font_t *desktop_font;
static x_theme_t desktop_theme;
static int selected_item = -1;
static uint32_t click_count;
static int screen_width;
static int screen_height;
static int last_status_second = -1;
static uint64_t last_screen_check_ms;

uint32_t ewok_xdesktop_click_count(void) {
    return click_count;
}

static void dock_repaint(xwin_t *window, graph_t *g) {
    (void)window;
    graph_clear(g, 0x00000000);
    graph_fill_round(g, 0, 0, g->w, g->h, 16, 0xcc202833);
    graph_round(g, 0, 0, g->w, g->h, 16, 1, 0x99ffffff);
    for(int i = 0; i < DOCK_ITEMS; i++) {
        int x = i * DOCK_ITEM_SIZE;
        if(i == selected_item) {
            graph_fill_round(g, x + 3, 3, DOCK_ITEM_SIZE - 6,
                    DOCK_ITEM_SIZE - 6, 12, 0xcc315c7a);
            graph_round(g, x + 3, 3, DOCK_ITEM_SIZE - 6,
                    DOCK_ITEM_SIZE - 6, 12, 2, 0xff75c7ff);
        }
        graph_t *icon = dock_items[i].icon;
        if(icon != NULL) {
            int ix = x + (DOCK_ITEM_SIZE - DOCK_ICON_SIZE) / 2;
            graph_blt_fit_alpha(icon, 0, 0, icon->w, icon->h, g,
                    ix, 5, DOCK_ICON_SIZE, DOCK_ICON_SIZE, 0xff);
        }
        if(desktop_font != NULL)
            graph_draw_text_font_align(g, x, 46, DOCK_ITEM_SIZE, 14,
                    dock_items[i].name, desktop_font,
                    desktop_theme.fontSize, 0xffeeeeee, FONT_ALIGN_CENTER);
        if(i == selected_item)
            graph_fill_circle(g, x + DOCK_ITEM_SIZE / 2,
                    DOCK_ITEM_SIZE - 3, 2, 0xff42a5f5);
    }
}

static void dock_event(xwin_t *window, xevent_t *event) {
    if(event->type != XEVT_MOUSE || event->state != MOUSE_STATE_CLICK)
        return;
    /* X server mouse coordinates are desktop-absolute.  Convert them to the
     * launcher's workspace before resolving a dock item. */
    int local_x = event->value.mouse.x - window->xinfo->wsr.x;
    int item = local_x / DOCK_ITEM_SIZE;
    if(item >= 0 && item < DOCK_ITEMS) {
        click_count++;
        selected_item = item;
        xwin_repaint(dock_window);
        int result = x_exec(dock_items[item].program);
        klog("xdesktop.wasm: clicked %s (%s, result=%d)\n",
                dock_items[item].name, dock_items[item].program, result);
    }
}

static void status_repaint(xwin_t *window, graph_t *g) {
    (void)window;
    graph_clear(g, 0xaa101820);
    if(desktop_font == NULL)
        return;
    graph_draw_text_font(g, 6, 2, "EwokOS micro-kernel", desktop_font,
            desktop_theme.fontSize, 0xffeeeeee);
    time_t now = time(NULL);
    struct tm current;
    char clock_text[16] = "--:--:--";
    if(localtime_r(&now, &current) != NULL)
        snprintf(clock_text, sizeof(clock_text), "%02d:%02d:%02d",
                current.tm_hour, current.tm_min, current.tm_sec);
    graph_draw_text_font_align(g, g->w - 108, 2, 96, 16, clock_text,
            desktop_font, desktop_theme.fontSize, 0xffeeeeee,
            FONT_ALIGN_CENTER);
}

int ewok_service_init(void) {
    x_init(&desktop_x, NULL);
    x_get_theme(&desktop_theme);
    desktop_font = font_new(desktop_theme.fontName, true);
    for(int i = 0; i < DOCK_ITEMS; i++)
        dock_items[i].icon = graph_image_new(dock_items[i].icon_path);

    xscreen_info_t screen;
    if(x_screen_info(&screen, 0) != 0)
        return -1;
    screen_width = screen.size.w;
    screen_height = screen.size.h;
    int dock_width = DOCK_ITEMS * DOCK_ITEM_SIZE;
    int dock_x = (screen.size.w - dock_width) / 2;
    int dock_y = screen.size.h - DOCK_ITEM_SIZE - 8;
    dock_window = xwin_open(&desktop_x, -1, dock_x, dock_y,
            dock_width, DOCK_ITEM_SIZE, "xlauncher",
            XWIN_STYLE_NO_TITLE | XWIN_STYLE_LAUNCHER |
            XWIN_STYLE_NO_BG_EFFECT);
    status_window = xwin_open(&desktop_x, -1, 0, 0,
            screen.size.w, STATUS_HEIGHT, "statusbar",
            XWIN_STYLE_NO_FOCUS | XWIN_STYLE_SYSBOTTOM |
            XWIN_STYLE_NO_FRAME | XWIN_STYLE_NO_BG_EFFECT);
    if(dock_window == NULL || status_window == NULL)
        return -1;
    dock_window->on_repaint = dock_repaint;
    dock_window->on_event = dock_event;
    status_window->on_repaint = status_repaint;
    xwin_set_alpha(dock_window, true);
    xwin_set_alpha(status_window, true);
    xwin_set_visible(status_window, true);
    xwin_set_visible(dock_window, true);
    klog("xdesktop.wasm: EwokOS wallpaper, statusbar and xlauncher ready\n");
    return 0;
}

int ewok_service_step(void) {
    if(dock_window == NULL || status_window == NULL)
        return -1;
    for(int i = 0; i < 8 && x_run_once(&desktop_x, NULL) == 0; i++) {}

    int resized = 0;
    uint64_t now_ms = kernel_tic_ms(0);
    if(now_ms - last_screen_check_ms >= 250) {
        last_screen_check_ms = now_ms;
        xscreen_info_t screen;
        if(x_screen_info(&screen, 0) == 0 &&
                (screen.size.w != screen_width || screen.size.h != screen_height)) {
            screen_width = screen.size.w;
            screen_height = screen.size.h;
            int dock_width = DOCK_ITEMS * DOCK_ITEM_SIZE;
            xwin_resize_to(status_window, screen_width, STATUS_HEIGHT);
            xwin_move_to(status_window, 0, 0);
            xwin_move_to(dock_window, (screen_width - dock_width) / 2,
                    screen_height - DOCK_ITEM_SIZE - 8);
            resized = 1;
        }
    }
    time_t now = time(NULL);
    struct tm current;
    if(localtime_r(&now, &current) != NULL &&
            current.tm_sec != last_status_second) {
        last_status_second = current.tm_sec;
        xwin_repaint(status_window);
    }
    if(resized)
        xwin_repaint(dock_window);
    return 0;
}
