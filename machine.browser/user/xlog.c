#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <ewoksys/kernel_tic.h>
#include <font/font.h>
#include <graph/graph_ex.h>
#include <x/x.h>
#include <x/xwin.h>

#define LOG_LINES 22
#define LOG_LINE_SIZE 96

static x_t app_x;
static xwin_t *app_window;
static font_t *app_font;
static x_theme_t app_theme;
static int log_fd = -1;
static char lines[LOG_LINES][LOG_LINE_SIZE];
static uint32_t line_count;
static char pending[LOG_LINE_SIZE];
static uint32_t pending_size;
static uint64_t last_read_ms;

static void push_line(void) {
    if(line_count == LOG_LINES) {
        memmove(lines, lines + 1, sizeof(lines) - sizeof(lines[0]));
        line_count--;
    }
    pending[pending_size] = 0;
    strncpy(lines[line_count++], pending, LOG_LINE_SIZE - 1);
    pending_size = 0;
}

static int read_log(void) {
    if(log_fd < 0)
        return 0;
    char buffer[512];
    int size = read(log_fd, buffer, sizeof(buffer));
    if(size <= 0)
        return 0;
    for(int i = 0; i < size; i++) {
        char value = buffer[i];
        if(value == '\r')
            continue;
        if(value == '\n') {
            push_line();
            continue;
        }
        if(pending_size == LOG_LINE_SIZE - 1)
            push_line();
        pending[pending_size++] = value;
    }
    return 1;
}

static void repaint(xwin_t *window, graph_t *g) {
    (void)window;
    graph_fill_rect(g, 0, 0, g->w, g->h, 0xff101820);
    graph_fill_rect(g, 0, 0, g->w, 24, 0xff263747);
    graph_draw_text_font(g, 8, 4, "Kernel log  /dev/log", app_font,
            app_theme.fontSize, 0xff7ee787);
    int line_height = app_theme.fontSize + 3;
    int visible = (g->h - 32) / line_height;
    if(visible > LOG_LINES)
        visible = LOG_LINES;
    int first = line_count > (uint32_t)visible ?
            (int)line_count - visible : 0;
    int y = 30;
    for(uint32_t i = (uint32_t)first; i < line_count; i++, y += line_height)
        graph_draw_text_font(g, 8, y, lines[i], app_font,
                app_theme.fontSize, 0xffd7e3ea);
}

int ewok_service_init(void) {
    x_init(&app_x, NULL);
    x_get_theme(&app_theme);
    app_font = font_new(app_theme.fontName, true);
    log_fd = open("/dev/log", O_RDONLY | O_NONBLOCK);
    app_window = xwin_open(&app_x, -1, 160, 100, 620, 360,
            "xlog.wasm", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
    if(app_window == NULL || app_font == NULL)
        return -1;
    app_window->on_repaint = repaint;
    x_set_app_name(&app_x, "/apps/xlog/xlog");
    read_log();
    last_read_ms = kernel_tic_ms(0);
    xwin_set_visible(app_window, true);
    return 0;
}

int ewok_service_step(void) {
    if(app_window == NULL)
        return -1;
    for(int i = 0; i < 8 && x_run_once(&app_x, NULL) == 0; i++) {}
    if(app_x.terminated || app_window->xinfo == NULL)
        return 0;
    uint64_t now = kernel_tic_ms(0);
    if(now - last_read_ms >= 250) {
        last_read_ms = now;
        if(read_log())
            xwin_repaint(app_window);
    }
    return 0;
}
