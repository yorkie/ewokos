#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ewoksys/kernel_tic.h>
#include <ewoksys/syscall.h>
#include <font/font.h>
#include <graph/graph_ex.h>
#include <procinfo.h>
#include <syscalls.h>
#include <x/x.h>
#include <x/xwin.h>

#define MAX_VISIBLE_PROCS 18

static x_t app_x;
static xwin_t *app_window;
static font_t *app_font;
static x_theme_t app_theme;
static procinfo_t *procs;
static int proc_count;
static uint64_t last_sample_ms;

static const char *state_name(int state) {
    static const char *names[] = {
        "unu", "crt", "slp", "wat", "blk", "rdy", "run", "zmb"
    };
    if(state < 0 || state >= (int)(sizeof(names) / sizeof(names[0])))
        return "unk";
    return names[state];
}

static void sample_processes(void) {
    int count = syscall0(SYS_GET_PROCS_NUM);
    if(count <= 0)
        return;
    procinfo_t *next = malloc(sizeof(procinfo_t) * (uint32_t)count);
    if(next == NULL)
        return;
    int got = syscall2(SYS_GET_PROCS, (ewokos_addr_t)count,
            (ewokos_addr_t)next);
    if(got < 0) {
        free(next);
        return;
    }
    free(procs);
    procs = next;
    proc_count = got;
}

static void repaint(xwin_t *window, graph_t *g) {
    (void)window;
    graph_fill_rect(g, 0, 0, g->w, g->h, 0xffe9edf0);
    graph_fill_rect(g, 0, 0, g->w, 26, 0xff263747);
    graph_draw_text_font(g, 8, 5, "PID", app_font,
            app_theme.fontSize, 0xffffffff);
    graph_draw_text_font(g, 58, 5, "CPU", app_font,
            app_theme.fontSize, 0xffffffff);
    graph_draw_text_font(g, 112, 5, "STATE", app_font,
            app_theme.fontSize, 0xffffffff);
    graph_draw_text_font(g, 178, 5, "HEAP", app_font,
            app_theme.fontSize, 0xffffffff);
    graph_draw_text_font(g, 260, 5, "COMMAND", app_font,
            app_theme.fontSize, 0xffffffff);

    int row = 0;
    for(int i = 0; i < proc_count && row < MAX_VISIBLE_PROCS; i++) {
        if(procs[i].type == TASK_TYPE_THREAD)
            continue;
        int y = 30 + row * 19;
        if((row & 1) != 0)
            graph_fill_rect(g, 0, y - 2, g->w, 19, 0xffdce3e8);
        if(procs[i].state == RUNNING)
            graph_fill_rect(g, 0, y - 2, 4, 19, 0xff42a5f5);
        char value[64];
        snprintf(value, sizeof(value), "%d", procs[i].pid);
        graph_draw_text_font(g, 8, y, value, app_font,
                app_theme.fontSize, 0xff17212b);
        snprintf(value, sizeof(value), "%u:%u%%", procs[i].core,
                procs[i].run_usec / 10000);
        graph_draw_text_font(g, 58, y, value, app_font,
                app_theme.fontSize, 0xff17212b);
        graph_draw_text_font(g, 112, y, state_name(procs[i].state), app_font,
                app_theme.fontSize, 0xff17212b);
        snprintf(value, sizeof(value), "%uK",
                (uint32_t)procs[i].heap_size / 1024);
        graph_draw_text_font(g, 178, y, value, app_font,
                app_theme.fontSize, 0xff17212b);
        graph_draw_text_font(g, 260, y, procs[i].cmd, app_font,
                app_theme.fontSize, 0xff17212b);
        row++;
    }
}

int ewok_service_init(void) {
    x_init(&app_x, NULL);
    x_get_theme(&app_theme);
    app_font = font_new(app_theme.fontName, true);
    app_window = xwin_open(&app_x, -1, 90, 76, 720, 390,
            "xprocs.wasm", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
    if(app_window == NULL || app_font == NULL)
        return -1;
    app_window->on_repaint = repaint;
    x_set_app_name(&app_x, "/apps/xprocs/xprocs");
    sample_processes();
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
    if(now - last_sample_ms >= 1000) {
        last_sample_ms = now;
        sample_processes();
        xwin_repaint(app_window);
    }
    return 0;
}
