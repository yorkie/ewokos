#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <ewoksys/kernel_tic.h>
#include <ewoksys/keydef.h>
#include <font/font.h>
#include <graph/graph_ex.h>
#include <mouse/mouse.h>
#include <x/x.h>
#include <x/xwin.h>

#define CONTENT_CAPACITY 65536
#define TOOLBAR_HEIGHT 36
#define STATUS_HEIGHT 24
#define TEXT_PADDING 10

extern int32_t wasm_host_launch_argument(char *buffer, uint32_t capacity);
extern uint32_t wasm_host_launch_generation(void);

static x_t app_x;
static xwin_t *app_window;
static font_t *app_font;
static x_theme_t app_theme;
static char content[CONTENT_CAPACITY];
static uint32_t content_size;
static uint32_t scroll_line;
static uint32_t total_lines;
static uint32_t font_size = 14;
static char current_file[FS_FULL_NAME_MAX + 1];
static char status_text[128];
static uint64_t last_argument_check_ms;
static uint32_t launch_generation;

static void draw_button(graph_t *g, int x, int y, int w, const char *label) {
    graph_fill_round(g, x, y, w, 26, 7, 0xff315c7au);
    graph_round(g, x, y, w, 26, 7, 1, 0x88ffffffu);
    graph_draw_text_font_align(g, x, y + 1, w, 24, label, app_font,
            app_theme.fontSize, 0xffffffffu, FONT_ALIGN_CENTER);
}

static uint32_t advance_line(uint32_t offset, uint32_t columns) {
    uint32_t used = 0;
    while(offset < content_size && used < columns) {
        char value = content[offset++];
        if(value == '\n')
            break;
        if(value != '\r')
            used++;
    }
    return offset;
}

static uint32_t line_offset(uint32_t line, uint32_t columns) {
    uint32_t offset = 0;
    while(line-- > 0 && offset < content_size)
        offset = advance_line(offset, columns);
    return offset;
}

static uint32_t count_lines(uint32_t columns) {
    uint32_t count = 0;
    uint32_t offset = 0;
    while(offset < content_size) {
        offset = advance_line(offset, columns);
        count++;
    }
    return count > 0 ? count : 1;
}

static void load_file(const char *path) {
    int fd = open(path, O_RDONLY);
    if(fd < 0) {
        snprintf(status_text, sizeof(status_text), "Cannot open %s", path);
        return;
    }
    content_size = 0;
    while(content_size < CONTENT_CAPACITY - 1) {
        int size = read(fd, content + content_size,
                CONTENT_CAPACITY - 1 - content_size);
        if(size <= 0)
            break;
        content_size += (uint32_t)size;
    }
    close(fd);
    content[content_size] = 0;
    memset(current_file, 0, sizeof(current_file));
    strncpy(current_file, path, sizeof(current_file) - 1);
    scroll_line = 0;
    snprintf(status_text, sizeof(status_text), "%s · %u bytes",
            current_file, content_size);
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
    load_file(value);
    return 1;
}

static void repaint(xwin_t *window, graph_t *g) {
    (void)window;
    graph_clear(g, 0xfff1f3f5u);
    graph_fill_rect(g, 0, 0, g->w, TOOLBAR_HEIGHT, 0xff263440u);
    draw_button(g, 8, 5, 42, "A+");
    draw_button(g, 56, 5, 42, "A-");
    graph_draw_text_font(g, 110, 10,
            current_file[0] != 0 ? current_file : "xread.wasm",
            app_font, app_theme.fontSize, 0xffeeeeeeu);

    uint32_t char_width = font_size > 7 ? font_size * 3 / 5 : 5;
    uint32_t columns = (g->w - TEXT_PADDING * 2) / char_width;
    if(columns < 8)
        columns = 8;
    uint32_t line_height = font_size + 5;
    uint32_t visible = (g->h - TOOLBAR_HEIGHT - STATUS_HEIGHT - 8) /
            line_height;
    if(visible < 1)
        visible = 1;
    total_lines = count_lines(columns);
    if(scroll_line >= total_lines)
        scroll_line = total_lines - 1;
    uint32_t offset = line_offset(scroll_line, columns);
    int y = TOOLBAR_HEIGHT + 6;
    for(uint32_t row = 0; row < visible && offset < content_size; row++) {
        char line[192];
        uint32_t used = 0;
        while(offset < content_size && used < columns &&
                used < sizeof(line) - 1) {
            char value = content[offset++];
            if(value == '\n')
                break;
            if(value == '\r')
                continue;
            line[used++] = value == '\t' ? ' ' : value;
        }
        line[used] = 0;
        graph_draw_text_font(g, TEXT_PADDING, y, line, app_font,
                font_size, 0xff202830u);
        y += line_height;
    }

    graph_fill_rect(g, 0, g->h - STATUS_HEIGHT, g->w,
            STATUS_HEIGHT, 0xff263440u);
    char footer[160];
    uint32_t percent = total_lines <= 1 ? 100 :
            (scroll_line * 100 / (total_lines - 1));
    snprintf(footer, sizeof(footer), "%s · %u%% · font %u",
            status_text, percent, font_size);
    graph_draw_text_font(g, 8, g->h - STATUS_HEIGHT + 5, footer,
            app_font, app_theme.fontSize, 0xffd7e3eau);
}

static void scroll_by(int lines) {
    if(lines < 0) {
        uint32_t amount = (uint32_t)(-lines);
        scroll_line = scroll_line < amount ? 0 : scroll_line - amount;
    }
    else {
        scroll_line += (uint32_t)lines;
    }
    if(total_lines > 0 && scroll_line >= total_lines)
        scroll_line = total_lines - 1;
}

static void event(xwin_t *window, xevent_t *event_value) {
    if(event_value->type == XEVT_MOUSE) {
        if(event_value->state == MOUSE_STATE_CLICK) {
            gpos_t pos = xwin_get_inside_pos(window,
                    event_value->value.mouse.x, event_value->value.mouse.y);
            if(pos.y < TOOLBAR_HEIGHT && pos.x >= 8 && pos.x < 50 &&
                    font_size < 32)
                font_size += 2;
            else if(pos.y < TOOLBAR_HEIGHT && pos.x >= 56 && pos.x < 98 &&
                    font_size > 8)
                font_size -= 2;
        }
        else if(event_value->value.mouse.button == MOUSE_BUTTON_SCROLL_UP)
            scroll_by(-3);
        else if(event_value->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN)
            scroll_by(3);
        xwin_repaint(window);
    }
    else if(event_value->type == XEVT_IM &&
            event_value->state == XIM_STATE_PRESS) {
        if(event_value->value.im.value == KEY_UP)
            scroll_by(-1);
        else if(event_value->value.im.value == KEY_DOWN)
            scroll_by(1);
        else if(event_value->value.im.value == KEY_HOME)
            scroll_line = 0;
        else if(event_value->value.im.value == KEY_END)
            scroll_line = total_lines > 0 ? total_lines - 1 : 0;
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
    app_window = xwin_open(&app_x, -1, 120, 80, 720, 480,
            "xread.wasm", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
    if(app_window == NULL)
        return -1;
    app_window->on_repaint = repaint;
    app_window->on_event = event;
    x_set_app_name(&app_x, "/apps/xread/xread");
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
    snprintf(status_text, sizeof(status_text), "No file selected");
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
