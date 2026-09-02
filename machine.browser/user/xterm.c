#include <stdint.h>
#include <string.h>

#include <ewoksys/keydef.h>
#include <font/font.h>
#include <graph/graph_ex.h>
#include <gterminal/gterminal.h>
#include <x/x.h>
#include <x/xwin.h>

#define TERM_BUFFER_SIZE 16384u
#define TERM_PADDING 10
#define TERM_HEADER_HEIGHT 24

extern int32_t wasm_host_tty_input(const void *data, uint32_t size);

static x_t app_x;
static xwin_t *app_window;
static gterminal_t terminal;
static x_theme_t app_theme;
static char pending[TERM_BUFFER_SIZE];
static uint32_t pending_read;
static uint32_t pending_write;
static uint32_t input_count;
static int terminal_width;
static int terminal_height;

/* Called by the browser Host while another EwokOS wasm process writes tty0.
 * Keep this callback syscall-free: it may run between cooperative process
 * slices while all modules share the same WebAssembly memory. */
int32_t ewok_xterm_write(const char *data, uint32_t size) {
    if(data == NULL || size == 0)
        return 0;
    uint32_t copied = 0;
    while(copied < size) {
        uint32_t next = (pending_write + 1) % TERM_BUFFER_SIZE;
        if(next == pending_read)
            pending_read = (pending_read + 1) % TERM_BUFFER_SIZE;
        pending[pending_write] = data[copied++];
        pending_write = next;
    }
    return (int32_t)copied;
}

uint32_t ewok_xterm_input_count(void) {
    return input_count;
}

static int drain_output(void) {
    char chunk[512];
    uint32_t size = 0;
    while(pending_read != pending_write && size < sizeof(chunk)) {
        chunk[size++] = pending[pending_read];
        pending_read = (pending_read + 1) % TERM_BUFFER_SIZE;
    }
    if(size == 0)
        return 0;
    gterminal_put(&terminal, chunk, (int)size);
    gterminal_scroll(&terminal, 0);
    return 1;
}

static void resize_terminal(int width, int height) {
    width -= TERM_PADDING * 2;
    height -= TERM_HEADER_HEIGHT + TERM_PADDING;
    if(width < 1)
        width = 1;
    if(height < 1)
        height = 1;
    if(width == terminal_width && height == terminal_height)
        return;
    terminal_width = width;
    terminal_height = height;
    gterminal_resize(&terminal, (uint32_t)width, (uint32_t)height);
}

static void repaint(xwin_t *window, graph_t *g) {
    (void)window;
    resize_terminal(g->w, g->h);
    graph_fill_rect(g, 0, 0, g->w, g->h, terminal.bg_color);
    graph_fill_rect(g, 0, 0, g->w, TERM_HEADER_HEIGHT, 0xff263747);
    graph_draw_text_font(g, TERM_PADDING, 4,
            "xterm.wasm  tty0", terminal.font, app_theme.fontSize,
            0xff7ee787);
    gterminal_paint(&terminal, g, TERM_PADDING, TERM_HEADER_HEIGHT,
            terminal_width, terminal_height);
}

static void on_resize(xwin_t *window) {
    if(window == NULL || window->xinfo == NULL)
        return;
    resize_terminal(window->xinfo->wsr.w, window->xinfo->wsr.h);
}

static void send_key(int value) {
    if(value <= 0 || value > 0xff || value == KEY_LSHIFT ||
            value == KEY_RSHIFT || value == KEY_CTRL)
        return;
    uint8_t key = value == KEY_ENTER ? '\n' : (uint8_t)value;
    if(wasm_host_tty_input(&key, 1) == 1)
        input_count++;
}

static void on_event(xwin_t *window, xevent_t *event) {
    if(event->type == XEVT_IM && event->state == XIM_STATE_PRESS) {
        send_key(event->value.im.value);
        gterminal_scroll(&terminal, 0);
        xwin_repaint(window);
    }
    else if(event->type == XEVT_MOUSE) {
        if(event->value.mouse.button == MOUSE_BUTTON_SCROLL_UP) {
            if(gterminal_scroll(&terminal, -1))
                xwin_repaint(window);
        }
        else if(event->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
            if(gterminal_scroll(&terminal, 1))
                xwin_repaint(window);
        }
        else if(event->state == MOUSE_STATE_CLICK) {
            xwin_call_xim(window, true);
        }
    }
}

int ewok_service_init(void) {
    x_init(&app_x, NULL);
    x_get_theme(&app_theme);
    gterminal_init(&terminal);
    terminal.font = font_new(app_theme.fontName, true);
    terminal.font_size = app_theme.fontSize > 0 ? app_theme.fontSize : 14;
    terminal.char_space = 1;
    terminal.line_space = 2;
    terminal.fg_color = 0xffd7e3ea;
    terminal.bg_color = 0xff101820;
    terminal.transparent = 0xff;
    gterminal_set_max_rows(&terminal, 2048);
    if(terminal.font == NULL)
        return -1;

    app_window = xwin_open(&app_x, -1, 110, 80, 700, 430,
            "xterm.wasm", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
    if(app_window == NULL)
        return -1;
    app_window->on_repaint = repaint;
    app_window->on_resize = on_resize;
    app_window->on_event = on_event;
    x_set_app_name(&app_x, "/apps/xterm/xterm");
    resize_terminal(app_window->xinfo->wsr.w, app_window->xinfo->wsr.h);
    static const char banner[] =
            "\033[1;36mEwokOS xterm on WebAssembly\033[0m\n"
            "connected to tty0\n";
    gterminal_put(&terminal, banner, sizeof(banner) - 1);
    xwin_set_visible(app_window, true);
    xwin_call_xim(app_window, true);
    return 0;
}

int ewok_service_step(void) {
    if(app_window == NULL)
        return -1;
    for(int i = 0; i < 8 && x_run_once(&app_x, NULL) == 0; i++) {}
    if(app_x.terminated || app_window->xinfo == NULL)
        return 0;
    int updated = 0;
    for(int i = 0; i < 32 && drain_output(); i++)
        updated = 1;
    if(updated)
        xwin_repaint(app_window);
    return 0;
}
