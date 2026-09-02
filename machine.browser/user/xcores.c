#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ewoksys/kernel_tic.h>
#include <ewoksys/sys.h>
#include <ewoksys/syscall.h>
#include <font/font.h>
#include <graph/graph_ex.h>
#include <sysinfo.h>
#include <x/x.h>
#include <x/xwin.h>

#define HISTORY_POINTS 64
#define SAMPLE_INTERVAL_MS 500

static x_t app_x;
static xwin_t *app_window;
static font_t *app_font;
static x_theme_t app_theme;
static sys_info_t sys_info;
static sys_state_t sys_state;
static uint8_t cpu_history[HISTORY_POINTS];
static uint8_t mem_history[HISTORY_POINTS];
static uint32_t history_count;
static uint32_t history_next;
static uint64_t last_sample_ms;

static uint32_t clamp_percent(uint64_t value) {
    return value > 100 ? 100 : (uint32_t)value;
}

static void sample_system(void) {
    if(sys_get_sys_info(&sys_info) != 0)
        memset(&sys_info, 0, sizeof(sys_info));
    if(syscall1(SYS_GET_SYS_STATE, (ewokos_addr_t)&sys_state) != 0)
        memset(&sys_state, 0, sizeof(sys_state));

    uint32_t idle = sys_info.cores == 0 ? 0 : sys_info.core_idles[0] / 10000;
    uint32_t cpu = sys_info.cores == 0 ? 0 : 100 - clamp_percent(idle);
    uint64_t total = sys_info.total_usable_mem_size;
    uint64_t used = total > sys_state.mem.free ? total - sys_state.mem.free : 0;
    uint32_t mem = total == 0 ? 0 : clamp_percent(used * 100 / total);

    cpu_history[history_next] = (uint8_t)cpu;
    mem_history[history_next] = (uint8_t)mem;
    history_next = (history_next + 1) % HISTORY_POINTS;
    if(history_count < HISTORY_POINTS)
        history_count++;
}

static uint32_t history_value(const uint8_t *history, uint32_t position) {
    uint32_t first = history_count < HISTORY_POINTS ? 0 : history_next;
    return history[(first + position) % HISTORY_POINTS];
}

static void draw_grid(graph_t *g, int x, int y, int w, int h) {
    graph_fill_rect(g, x, y, w, h, 0xff18222c);
    for(int i = 0; i <= 4; i++) {
        int gy = y + i * h / 4;
        graph_line(g, x, gy, x + w - 1, gy, 0xff344451);
    }
    for(int i = 0; i <= 8; i++) {
        int gx = x + i * w / 8;
        graph_line(g, gx, y, gx, y + h - 1, 0xff2a3945);
    }
}

static void draw_history(graph_t *g, const uint8_t *history, uint32_t color,
        int x, int y, int w, int h) {
    if(history_count < 2)
        return;
    int last_x = x;
    int last_y = y + h - 1 - (int)history_value(history, 0) * (h - 1) / 100;
    for(uint32_t i = 1; i < history_count; i++) {
        int px = x + (int)i * (w - 1) / (HISTORY_POINTS - 1);
        int py = y + h - 1 - (int)history_value(history, i) * (h - 1) / 100;
        graph_wline(g, last_x, last_y, px, py, 2, color);
        last_x = px;
        last_y = py;
    }
}

static void repaint(xwin_t *window, graph_t *g) {
    (void)window;
    graph_fill_rect(g, 0, 0, g->w, g->h, 0xff202b35);

    int margin = 12;
    int label_h = 24;
    int gap = 12;
    int graph_h = (g->h - margin * 2 - label_h * 2 - gap) / 2;
    if(graph_h < 24)
        graph_h = 24;
    int graph_w = g->w - margin * 2;
    int cpu_y = margin + label_h;
    int mem_label_y = cpu_y + graph_h + gap;
    int mem_y = mem_label_y + label_h;

    uint32_t cpu = history_count == 0 ? 0 :
            history_value(cpu_history, history_count - 1);
    uint32_t mem = history_count == 0 ? 0 :
            history_value(mem_history, history_count - 1);
    char label[96];
    snprintf(label, sizeof(label), "CPU core 0  %u%%", cpu);
    graph_draw_text_font(g, margin, margin + 3, label, app_font,
            app_theme.fontSize, 0xff68c8ff);
    draw_grid(g, margin, cpu_y, graph_w, graph_h);
    draw_history(g, cpu_history, 0xff42a5f5, margin, cpu_y, graph_w, graph_h);

    uint32_t total_mb = (uint32_t)(sys_info.total_usable_mem_size / (1024 * 1024));
    uint32_t used_mb = total_mb * mem / 100;
    snprintf(label, sizeof(label), "Memory  %u/%u MiB  %u%%",
            used_mb, total_mb, mem);
    graph_draw_text_font(g, margin, mem_label_y + 3, label, app_font,
            app_theme.fontSize, 0xff7ee787);
    draw_grid(g, margin, mem_y, graph_w, graph_h);
    draw_history(g, mem_history, 0xff7ee787, margin, mem_y, graph_w, graph_h);
}

int ewok_service_init(void) {
    memset(&app_x, 0, sizeof(app_x));
    x_init(&app_x, NULL);
    x_get_theme(&app_theme);
    app_font = font_new(app_theme.fontName, true);
    app_window = xwin_open(&app_x, -1, 430, 150, 420, 300,
            "xcores.wasm", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
    if(app_window == NULL || app_font == NULL)
        return -1;
    app_window->on_repaint = repaint;
    x_set_app_name(&app_x, "/apps/xcores/xcores");
    sample_system();
    last_sample_ms = kernel_tic_ms(0);
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
    if(now - last_sample_ms >= SAMPLE_INTERVAL_MS) {
        last_sample_ms = now;
        sample_system();
        xwin_repaint(app_window);
    }
    return 0;
}
