#include "web_x.h"
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>

#ifdef __cplusplus
extern "C" {
#endif

static xwin_t* active_windows[32] = {0}; // Simple window management
static int next_window_id = 1;
static char current_canvas[64] = "canvas";

// JavaScript event handlers
EM_JS(void, js_setup_mouse_events, (const char* canvas_id), {
    const canvas = document.getElementById(UTF8ToString(canvas_id));
    if (!canvas) return;
    
    const sendMouseEvent = (event, type, button = 0) => {
        const rect = canvas.getBoundingClientRect();
        const x = event.clientX - rect.left;
        const y = event.clientY - rect.top;
        Module.ccall('web_handle_mouse_event', null, 
                    ['number', 'number', 'number', 'number', 'number'], 
                    [type, x, y, button, event.buttons]);
    };
    
    canvas.addEventListener('mousedown', (e) => sendMouseEvent(e, 1, e.button));
    canvas.addEventListener('mouseup', (e) => sendMouseEvent(e, 2, e.button));
    canvas.addEventListener('mousemove', (e) => sendMouseEvent(e, 3, 0));
    canvas.addEventListener('click', (e) => sendMouseEvent(e, 4, e.button));
});

EM_JS(void, js_setup_keyboard_events, (const char* canvas_id), {
    const canvas = document.getElementById(UTF8ToString(canvas_id));
    if (!canvas) return;
    
    canvas.tabIndex = 0; // Make canvas focusable
    
    const sendKeyEvent = (event, type) => {
        Module.ccall('web_handle_key_event', null, 
                    ['number', 'number', 'number'], 
                    [type, event.keyCode, event.charCode]);
    };
    
    canvas.addEventListener('keydown', (e) => {
        sendKeyEvent(e, 1);
        e.preventDefault();
    });
    canvas.addEventListener('keyup', (e) => {
        sendKeyEvent(e, 2);
        e.preventDefault();
    });
});

// Event handler functions called from JavaScript
EMSCRIPTEN_KEEPALIVE
void web_handle_mouse_event(int type, int x, int y, int button, int buttons) {
    xevent_t ev;
    memset(&ev, 0, sizeof(ev));
    
    ev.type = XEVT_MOUSE;
    ev.value.mouse.x = x;
    ev.value.mouse.y = y;
    ev.value.mouse.button = button;
    
    switch (type) {
        case 1: ev.state = 1; break; // mouse down
        case 2: ev.state = 2; break; // mouse up
        case 3: ev.state = 3; break; // mouse move
        case 4: ev.state = 4; break; // click
    }
    
    // Send to active window
    for (int i = 0; i < 32; i++) {
        if (active_windows[i] && active_windows[i]->visible) {
            if (active_windows[i]->on_event) {
                active_windows[i]->on_event(active_windows[i], &ev);
            }
            break; // Only send to first active window for now
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void web_handle_key_event(int type, int key_code, int char_code) {
    xevent_t ev;
    memset(&ev, 0, sizeof(ev));
    
    ev.type = XEVT_IM;
    ev.state = (type == 1) ? XIM_STATE_PRESS : XIM_STATE_RELEASE;
    ev.value.im.value = char_code ? char_code : key_code;
    
    // Send to focused window
    for (int i = 0; i < 32; i++) {
        if (active_windows[i] && active_windows[i]->focused) {
            if (active_windows[i]->on_event) {
                active_windows[i]->on_event(active_windows[i], &ev);
            }
            break;
        }
    }
}

// Core X functions
void x_init(x_t* x, void* data) {
    memset(x, 0, sizeof(x_t));
    x->data = data;
    x->terminated = false;
}

static void main_loop(void* arg) {
    x_t* x = (x_t*)arg;
    
    if (x->terminated) {
        emscripten_cancel_main_loop();
        return;
    }
    
    if (x->on_loop) {
        x->on_loop(x->data);
    }
    
    // Repaint all visible windows
    for (int i = 0; i < 32; i++) {
        if (active_windows[i] && active_windows[i]->visible) {
            xwin_repaint(active_windows[i]);
        }
    }
}

int x_run(x_t* x, void* loop_data) {
    x->data = loop_data;
    emscripten_set_main_loop_arg(main_loop, x, 60, 1); // 60 FPS
    return 0;
}

void x_terminate(x_t* x) {
    x->terminated = true;
}

void x_push_event(x_t* x, xevent_t* ev) {
    // Simple event handling - could be enhanced with a queue
    if (x->main_win && x->main_win->on_event) {
        x->main_win->on_event(x->main_win, ev);
    }
}

// Window functions
xwin_t* xwin_open(x_t* xp, uint32_t disp_index, int x, int y, int w, int h, const char* title, int style) {
    xwin_t* xwin = (xwin_t*)malloc(sizeof(xwin_t));
    if (!xwin) return NULL;
    
    memset(xwin, 0, sizeof(xwin_t));
    xwin->x = xp;
    xwin->id = next_window_id++;
    xwin->x_pos = x;
    xwin->y_pos = y;
    xwin->width = w;
    xwin->height = h;
    xwin->visible = false;
    xwin->focused = false;
    
    if (title) {
        strncpy(xwin->title, title, sizeof(xwin->title) - 1);
    }
    
    // Create graphics buffer
    xwin->graph = graph_new(NULL, w, h);
    if (!xwin->graph) {
        free(xwin);
        return NULL;
    }
    
    // Add to active windows
    for (int i = 0; i < 32; i++) {
        if (!active_windows[i]) {
            active_windows[i] = xwin;
            break;
        }
    }
    
    if (!xp->main_win) {
        xp->main_win = xwin;
        xwin->focused = true;
    }
    
    return xwin;
}

void xwin_close(xwin_t* xwin) {
    if (!xwin) return;
    
    if (xwin->on_close && !xwin->on_close(xwin)) {
        return; // Close was cancelled
    }
    
    xwin_destroy(xwin);
}

void xwin_destroy(xwin_t* xwin) {
    if (!xwin) return;
    
    // Remove from active windows
    for (int i = 0; i < 32; i++) {
        if (active_windows[i] == xwin) {
            active_windows[i] = NULL;
            break;
        }
    }
    
    if (xwin->x && xwin->x->main_win == xwin) {
        xwin->x->main_win = NULL;
    }
    
    if (xwin->graph) {
        graph_free(xwin->graph);
    }
    
    free(xwin);
}

int xwin_set_visible(xwin_t* xwin, bool visible) {
    if (!xwin) return -1;
    
    xwin->visible = visible;
    if (visible) {
        xwin_repaint(xwin);
    }
    
    return 0;
}

void xwin_repaint(xwin_t* xwin) {
    if (!xwin || !xwin->visible || !xwin->graph) return;
    
    if (xwin->on_repaint) {
        xwin->on_repaint(xwin, xwin->graph);
    }
    
    // Flush to canvas
    web_graph_flush_to_canvas(xwin->graph, current_canvas);
}

graph_t* xwin_fetch_graph(xwin_t* xwin, graph_t* g) {
    if (!xwin) return NULL;
    
    if (g) {
        // Copy to provided graph
        memcpy(g, xwin->graph, sizeof(graph_t));
        return g;
    }
    
    return xwin->graph;
}

int xwin_resize_to(xwin_t* xwin, int w, int h) {
    if (!xwin) return -1;
    
    xwin->width = w;
    xwin->height = h;
    
    // Recreate graphics buffer
    if (xwin->graph) {
        graph_free(xwin->graph);
    }
    
    xwin->graph = graph_new(NULL, w, h);
    if (!xwin->graph) return -1;
    
    if (xwin->on_resize) {
        xwin->on_resize(xwin);
    }
    
    return 0;
}

int xwin_move_to(xwin_t* xwin, int x, int y) {
    if (!xwin) return -1;
    
    xwin->x_pos = x;
    xwin->y_pos = y;
    
    if (xwin->on_move) {
        xwin->on_move(xwin);
    }
    
    return 0;
}

// Web-specific functions
void web_x_init_canvas(const char* canvas_id) {
    strncpy(current_canvas, canvas_id, sizeof(current_canvas) - 1);
    current_canvas[sizeof(current_canvas) - 1] = '\0';
}

void web_x_setup_event_handlers(const char* canvas_id) {
    js_setup_mouse_events(canvas_id);
    js_setup_keyboard_events(canvas_id);
}

#ifdef __cplusplus
}
#endif