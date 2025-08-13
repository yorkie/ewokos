#ifndef WEB_X_H
#define WEB_X_H

#include "wasm_types.h"
#include "web_xevent.h"
#include "web_graph.h"

#ifdef __cplusplus
extern "C" {
#endif

struct st_xwin;

typedef struct {
    struct st_xwin* main_win;
    void* data;
    bool terminated;
    
    void (*on_loop)(void* p);
} x_t;

typedef struct st_xwin {
    x_t* x;
    int id;
    void* data;
    
    int32_t x_pos, y_pos;
    int32_t width, height;
    char title[256];
    bool visible;
    bool focused;
    
    graph_t* graph;
    
    // Event handlers
    bool (*on_close)(struct st_xwin* xwin);
    void (*on_resize)(struct st_xwin* xwin);
    void (*on_move)(struct st_xwin* xwin);
    void (*on_focus)(struct st_xwin* xwin);
    void (*on_unfocus)(struct st_xwin* xwin);
    void (*on_repaint)(struct st_xwin* xwin, graph_t* g);
    void (*on_event)(struct st_xwin* xwin, xevent_t* ev);
} xwin_t;

// Core X functions
void     x_init(x_t* x, void* data);
int      x_run(x_t* x, void* loop_data);
void     x_terminate(x_t* x);
void     x_push_event(x_t* x, xevent_t* ev);

// Window functions
xwin_t*  xwin_open(x_t* xp, uint32_t disp_index, int x, int y, int w, int h, const char* title, int style);
void     xwin_close(xwin_t* xwin);
void     xwin_destroy(xwin_t* xwin);
int      xwin_set_visible(xwin_t* xwin, bool visible);
void     xwin_repaint(xwin_t* xwin);
graph_t* xwin_fetch_graph(xwin_t* xwin, graph_t* g);
int      xwin_resize_to(xwin_t* xwin, int w, int h);
int      xwin_move_to(xwin_t* xwin, int x, int y);

// Web-specific functions
void     web_x_init_canvas(const char* canvas_id);
void     web_x_setup_event_handlers(const char* canvas_id);

#ifdef __cplusplus
}
#endif

#endif // WEB_X_H