#include <x/xwin.h>
#include <x/x.h>
#include <wasm_ewok_compat.h>
#include <font/font.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// WebAssembly global state
static int next_display_id = 0;
static char work_dir[256] = "/";

static int x_get_event(int xserv_pid, xevent_t* ev, bool block) {
    // WebAssembly event handling will be implemented via JavaScript callbacks
    return -1; // No events available
}

int x_screen_info(xscreen_info_t* scr, uint32_t index) {
    if (!scr || index >= 1) return -1;
    
    // Default WebAssembly canvas screen info
    scr->width = 800;
    scr->height = 600;
    scr->depth = 32;
    scr->dpi = 96;
    return 0;
}

int x_fetch_screen_graph(uint32_t index, graph_t* g) {
    if (!g || index >= 1) return -1;
    
    // Create a default canvas-sized graph
    g->w = 800;
    g->h = 600;
    g->buffer = (uint32_t*)malloc(g->w * g->h * sizeof(uint32_t));
    g->need_free = true;
    g->clip.x = 0;
    g->clip.y = 0;
    g->clip.w = g->w;
    g->clip.h = g->h;
    
    if (!g->buffer) {
        return -1;
    }
    memset(g->buffer, 0xff, g->w * g->h * sizeof(uint32_t)); // White background
    return 0;
}

int x_get_display_num(void) {
    return 1; // Single WebAssembly canvas display
}

void x_init(x_t* x, void* data) {
    if (!x) return;
    
    memset(x, 0, sizeof(x_t));
    x->data = data;
    x->terminated = false;
    x->main_win = NULL;
    x->prompt_win = NULL;
    x->event_head = NULL;
    x->event_tail = NULL;
    x->on_loop = NULL;
}

int x_run(x_t* x, void* loop_data) {
    if (!x) return -1;
    
    // WebAssembly event loop will be handled by browser
    // This is a stub that could be called from JavaScript
    while (!x->terminated) {
        if (x->on_loop) {
            x->on_loop(loop_data);
        }
        
        // Process events from queue
        x_event_t* ev = x->event_head;
        while (ev) {
            x_event_t* next = ev->next;
            
            // Handle the event - dispatch to window handlers
            if (x->main_win && x->main_win->on_event) {
                x->main_win->on_event(x->main_win, &ev->event);
            }
            
            free(ev);
            ev = next;
        }
        x->event_head = x->event_tail = NULL;
        
        // In WebAssembly, we yield control back to browser
#ifdef __EMSCRIPTEN__
        emscripten_sleep(16); // ~60fps
#endif
    }
    
    return 0;
}

void x_terminate(x_t* x) {
    if (!x) return;
    x->terminated = true;
}

void x_push_event(x_t* x, xevent_t* ev) {
    if (!x || !ev) return;
    
    x_event_t* xev = (x_event_t*)malloc(sizeof(x_event_t));
    if (!xev) return;
    
    memcpy(&xev->event, ev, sizeof(xevent_t));
    xev->next = NULL;
    
    if (x->event_tail) {
        x->event_tail->next = xev;
        x->event_tail = xev;
    } else {
        x->event_head = x->event_tail = xev;
    }
}

const char* x_get_work_dir(void) {
    return work_dir;
}

int x_exec(const char* fname) {
    // WebAssembly stub - cannot exec new processes
    return -1;
}

int x_set_top_app(const char* fname) {
    // WebAssembly stub
    return 0;
}

int x_set_app_name(x_t* x, const char* fname) {
    // WebAssembly stub
    return 0;
}

int x_show_cursor(bool show) {
    // WebAssembly cursor control via JavaScript
#ifdef __EMSCRIPTEN__
    EM_ASM({
        document.body.style.cursor = $0 ? 'default' : 'none';
    }, show);
#endif
    return 0;
}

int x_get_theme(x_theme_t* theme) {
    if (!theme) return -1;
    
    // Default WebAssembly theme
    theme->bgColor = 0xfff0f0f0;
    theme->fgColor = 0xff000000;
    theme->selectColor = 0xff0078d4;
    theme->fontSize = 14;
    return 0;
}

const char* x_get_theme_fname(const char* prefix, const char* app_name, const char* fname) {
    static char theme_fname[256];
    snprintf(theme_fname, sizeof(theme_fname), "%s/%s", prefix ? prefix : "", fname ? fname : "");
    return theme_fname;
}

int x_get_desktop_space(int disp_index, grect_t* r) {
    if (!r || disp_index >= 1) return -1;
    
    r->x = 0;
    r->y = 0;
    r->w = 800;
    r->h = 600;
    return 0;
}

int x_set_desktop_space(int disp_index, const grect_t* r) {
    // WebAssembly stub
    return 0;
}

const char* x_get_res_name(const char* name) {
    return name; // Pass through in WebAssembly
}

#ifdef __cplusplus
}
#endif