#include <x/xwin.h>
#include <wasm_ewok_compat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// WebAssembly window management
static int next_window_id = 1;
static xwin_t* active_windows[32] = {0}; // Simple window registry

// JavaScript canvas management functions
#ifdef __EMSCRIPTEN__
EM_JS(void, js_create_canvas, (int win_id, int x, int y, int w, int h, const char* title), {
    const canvas = document.createElement('canvas');
    canvas.id = 'xwin_' + win_id;
    canvas.width = w;
    canvas.height = h;
    canvas.style.position = 'absolute';
    canvas.style.left = x + 'px';
    canvas.style.top = y + 'px';
    canvas.style.border = '1px solid #ccc';
    canvas.style.zIndex = win_id;
    
    if (title) {
        canvas.title = UTF8ToString(title);
    }
    
    document.body.appendChild(canvas);
});

EM_JS(void, js_destroy_canvas, (int win_id), {
    const canvas = document.getElementById('xwin_' + win_id);
    if (canvas) {
        canvas.remove();
    }
});

EM_JS(void, js_show_canvas, (int win_id, bool visible), {
    const canvas = document.getElementById('xwin_' + win_id);
    if (canvas) {
        canvas.style.display = visible ? 'block' : 'none';
    }
});

EM_JS(void, js_move_canvas, (int win_id, int x, int y), {
    const canvas = document.getElementById('xwin_' + win_id);
    if (canvas) {
        canvas.style.left = x + 'px';
        canvas.style.top = y + 'px';
    }
});

EM_JS(void, js_resize_canvas, (int win_id, int w, int h), {
    const canvas = document.getElementById('xwin_' + win_id);
    if (canvas) {
        canvas.width = w;
        canvas.height = h;
    }
});
#else
// Stub implementations for non-Emscripten builds
static void js_create_canvas(int win_id, int x, int y, int w, int h, const char* title) {}
static void js_destroy_canvas(int win_id) {}
static void js_show_canvas(int win_id, bool visible) {}
static void js_move_canvas(int win_id, int x, int y) {}
static void js_resize_canvas(int win_id, int w, int h) {}
#endif

static int xwin_update_info(xwin_t* xwin, uint8_t type) {
    if (!xwin || !xwin->xinfo) return -1;
    
    // WebAssembly stub - no shared memory updates needed
    return 0;
}

void xwin_busy(xwin_t* xwin, bool busy) {
    // WebAssembly cursor busy state
#ifdef __EMSCRIPTEN__
    EM_ASM({
        document.body.style.cursor = $0 ? 'wait' : 'default';
    }, busy);
#endif
}

int xwin_call_xim(xwin_t* xwin, bool show) {
    // WebAssembly input method stub
    return 0;
}

int xwin_top(xwin_t* xwin) {
    if (!xwin || !xwin->xinfo) return -1;
    
    // Bring window to front by adjusting z-index
#ifdef __EMSCRIPTEN__
    EM_ASM({
        const canvas = document.getElementById('xwin_' + $0);
        if (canvas) {
            canvas.style.zIndex = 1000 + $0;
        }
    }, xwin->xinfo->win_id);
#endif
    
    return 0;
}

static int x_get_win_rect(int xfd, int style, grect_t* wsr, grect_t* win_space) {
    if (!wsr || !win_space) return -1;
    
    // WebAssembly - just copy the workspace rect
    memcpy(win_space, wsr, sizeof(grect_t));
    return 0;
}

xwin_t* xwin_open(x_t* xp, uint32_t disp_index, int x, int y, int w, int h, const char* title, int style) {
    if (w <= 0 || h <= 0) return NULL;
    
    if (disp_index >= x_get_display_num()) {
        disp_index = 0;
    }
    
    xwin_t* ret = (xwin_t*)malloc(sizeof(xwin_t));
    if (!ret) return NULL;
    
    memset(ret, 0, sizeof(xwin_t));
    ret->fd = next_window_id; // Use window ID as fake fd
    ret->x = xp;
    
    // Allocate xinfo structure directly (no shared memory in WebAssembly)
    ret->xinfo = (xinfo_t*)malloc(sizeof(xinfo_t));
    if (!ret->xinfo) {
        free(ret);
        return NULL;
    }
    
    memset(ret->xinfo, 0, sizeof(xinfo_t));
    ret->xinfo->win_id = next_window_id++;
    ret->xinfo->win = (uint32_t)ret;
    ret->xinfo->style = style;
    ret->xinfo->display_index = disp_index;
    ret->xinfo->state = XWIN_STATE_NORMAL;
    
    if ((style & XWIN_STYLE_PROMPT) != 0) {
        xp->prompt_win = ret;
    }
    
    if (xp->main_win == NULL) {
        ret->xinfo->is_main = true;
        xp->main_win = ret;
    }
    
    // Set window workspace rectangle
    ret->xinfo->wsr.x = x;
    ret->xinfo->wsr.y = y;
    ret->xinfo->wsr.w = w;
    ret->xinfo->wsr.h = h;
    
    // Set title
    if (title) {
        strncpy(ret->xinfo->title, title, XWIN_TITLE_MAX - 1);
        ret->xinfo->title[XWIN_TITLE_MAX - 1] = '\0';
    }
    
    // Allocate graphics buffer
    ret->xinfo->g.w = w;
    ret->xinfo->g.h = h;
    ret->xinfo->g.buffer = (uint32_t*)malloc(w * h * sizeof(uint32_t));
    ret->xinfo->g.need_free = true;
    ret->xinfo->g.clip.x = 0;
    ret->xinfo->g.clip.y = 0;
    ret->xinfo->g.clip.w = w;
    ret->xinfo->g.clip.h = h;
    
    if (!ret->xinfo->g.buffer) {
        free(ret->xinfo);
        free(ret);
        return NULL;
    }
    
    // Clear to white
    memset(ret->xinfo->g.buffer, 0xff, w * h * sizeof(uint32_t));
    
    // Create HTML canvas
    js_create_canvas(ret->xinfo->win_id, x, y, w, h, title);
    
    // Register window
    if (ret->xinfo->win_id < 32) {
        active_windows[ret->xinfo->win_id] = ret;
    }
    
    pthread_mutex_init(&ret->painting_lock, NULL);
    return ret;
}

int xwin_fullscreen(xwin_t* xwin) {
    if (!xwin || !xwin->xinfo) return -1;
    
    xwin->xinfo->style |= XWIN_STYLE_NO_RESIZE | XWIN_STYLE_NO_TITLE;
    return xwin_max(xwin);
}

int xwin_max(xwin_t* xwin) {
    if (!xwin || !xwin->xinfo) return -1;
    
    // Save current state
    memcpy(&xwin->xinfo_prev, xwin->xinfo, sizeof(xinfo_t));
    
    // Maximize to full canvas
    xwin->xinfo->state = XWIN_STATE_MAX;
    xwin->xinfo->wsr.x = 0;
    xwin->xinfo->wsr.y = 0;
    xwin->xinfo->wsr.w = 800; // Default canvas size
    xwin->xinfo->wsr.h = 600;
    
    js_resize_canvas(xwin->xinfo->win_id, 800, 600);
    js_move_canvas(xwin->xinfo->win_id, 0, 0);
    
    if (xwin->on_resize) {
        xwin->on_resize(xwin);
    }
    
    return 0;
}

static graph_t* x_get_graph(xwin_t* xwin, graph_t* g) {
    if (!xwin || !xwin->xinfo) return NULL;
    
    // Return the window's graphics buffer
    if (g) {
        memcpy(g, &xwin->xinfo->g, sizeof(graph_t));
        return g;
    }
    
    return &xwin->xinfo->g;
}

graph_t* xwin_fetch_graph(xwin_t* xwin, graph_t* g) {
    return x_get_graph(xwin, g);
}

void xwin_close(xwin_t* xwin) {
    if (!xwin) return;
    
    xwin->xinfo->closed = true;
    
    if (xwin->on_close) {
        if (!xwin->on_close(xwin)) {
            return; // Cancel close
        }
    }
    
    xwin_destroy(xwin);
}

void xwin_destroy(xwin_t* xwin) {
    if (!xwin) return;
    
    // Remove from active windows
    if (xwin->xinfo && xwin->xinfo->win_id < 32) {
        active_windows[xwin->xinfo->win_id] = NULL;
    }
    
    // Destroy HTML canvas
    if (xwin->xinfo) {
        js_destroy_canvas(xwin->xinfo->win_id);
    }
    
    // Clean up memory
    pthread_mutex_destroy(&xwin->painting_lock);
    
    if (xwin->xinfo) {
        if (xwin->xinfo->g.buffer && xwin->xinfo->g.need_free) {
            free(xwin->xinfo->g.buffer);
        }
        free(xwin->xinfo);
    }
    
    // Update parent X context
    if (xwin->x) {
        if (xwin->x->main_win == xwin) {
            xwin->x->main_win = NULL;
        }
        if (xwin->x->prompt_win == xwin) {
            xwin->x->prompt_win = NULL;
        }
    }
    
    free(xwin);
}

int xwin_set_visible(xwin_t* xwin, bool visible) {
    if (!xwin || !xwin->xinfo) return -1;
    
    xwin->xinfo->visible = visible;
    js_show_canvas(xwin->xinfo->win_id, visible);
    
    return 0;
}

void xwin_set_alpha(xwin_t* xwin, bool alpha) {
    if (!xwin || !xwin->xinfo) return;
    
    xwin->xinfo->alpha = alpha;
    
    // Set canvas transparency
#ifdef __EMSCRIPTEN__
    EM_ASM({
        const canvas = document.getElementById('xwin_' + $0);
        if (canvas) {
            canvas.style.opacity = $1 ? '0.8' : '1.0';
        }
    }, xwin->xinfo->win_id, alpha);
#endif
}

void xwin_repaint(xwin_t* xwin) {
    if (!xwin || !xwin->xinfo) return;
    
    pthread_mutex_lock(&xwin->painting_lock);
    
    if (xwin->on_repaint) {
        xwin->on_repaint(xwin, &xwin->xinfo->g);
    }
    
    // Copy buffer to canvas
    if (xwin->xinfo->g.buffer) {
#ifdef __EMSCRIPTEN__
        EM_ASM({
            const canvas = document.getElementById('xwin_' + $0);
            if (canvas) {
                const ctx = canvas.getContext('2d');
                const imageData = ctx.createImageData($1, $2);
                const buffer = new Uint8Array(Module.HEAPU8.buffer, $3, $1 * $2 * 4);
                
                // Convert ARGB to RGBA
                for (let i = 0; i < buffer.length; i += 4) {
                    imageData.data[i] = buffer[i + 2];     // R
                    imageData.data[i + 1] = buffer[i + 1]; // G
                    imageData.data[i + 2] = buffer[i];     // B
                    imageData.data[i + 3] = buffer[i + 3]; // A
                }
                
                ctx.putImageData(imageData, 0, 0);
            }
        }, xwin->xinfo->win_id, xwin->xinfo->g.w, xwin->xinfo->g.h, (uint32_t)xwin->xinfo->g.buffer);
#endif
    }
    
    pthread_mutex_unlock(&xwin->painting_lock);
}

int xwin_resize(xwin_t* xwin, int dw, int dh) {
    if (!xwin || !xwin->xinfo) return -1;
    
    return xwin_resize_to(xwin, xwin->xinfo->wsr.w + dw, xwin->xinfo->wsr.h + dh);
}

int xwin_resize_to(xwin_t* xwin, int w, int h) {
    if (!xwin || !xwin->xinfo || w <= 0 || h <= 0) return -1;
    
    // Reallocate graphics buffer
    if (xwin->xinfo->g.buffer && xwin->xinfo->g.need_free) {
        free(xwin->xinfo->g.buffer);
    }
    
    xwin->xinfo->g.w = w;
    xwin->xinfo->g.h = h;
    xwin->xinfo->g.buffer = (uint32_t*)malloc(w * h * sizeof(uint32_t));
    xwin->xinfo->g.clip.w = w;
    xwin->xinfo->g.clip.h = h;
    
    if (!xwin->xinfo->g.buffer) return -1;
    
    // Clear to white
    memset(xwin->xinfo->g.buffer, 0xff, w * h * sizeof(uint32_t));
    
    // Update window size
    xwin->xinfo->wsr.w = w;
    xwin->xinfo->wsr.h = h;
    
    // Resize HTML canvas
    js_resize_canvas(xwin->xinfo->win_id, w, h);
    
    if (xwin->on_resize) {
        xwin->on_resize(xwin);
    }
    
    return 0;
}

int xwin_move(xwin_t* xwin, int dx, int dy) {
    if (!xwin || !xwin->xinfo) return -1;
    
    return xwin_move_to(xwin, xwin->xinfo->wsr.x + dx, xwin->xinfo->wsr.y + dy);
}

int xwin_move_to(xwin_t* xwin, int x, int y) {
    if (!xwin || !xwin->xinfo) return -1;
    
    xwin->xinfo->wsr.x = x;
    xwin->xinfo->wsr.y = y;
    
    js_move_canvas(xwin->xinfo->win_id, x, y);
    
    if (xwin->on_move) {
        xwin->on_move(xwin);
    }
    
    return 0;
}

#ifdef __cplusplus
}
#endif