#include "web_x++.h"
#include <cstring>

namespace Ewok {

// X class implementation
X::X(void) {
    x_init(&x, this);
}

void X::run(void (*loop)(void*), void* p) {
    x_run(&x, p);
}

void X::terminate(void) {
    x_terminate(&x);
}

bool X::terminated(void) {
    return x.terminated;
}

const char* X::getResName(const char* name) {
    return name; // Simplified for web
}

bool X::getScreenInfo(int& w, int& h, int index) {
    w = 800; // Default canvas size
    h = 600;
    return true;
}

// XWin class implementation
XWin::XWin() : xwin(nullptr) {
}

XWin::~XWin() {
    if (xwin) {
        close();
    }
}

bool XWin::open(X* x, uint32_t disp_index, int x_pos, int y_pos, int w, int h, const char* title, int style) {
    xwin = xwin_open(x->c_x(), disp_index, x_pos, y_pos, w, h, title, style);
    
    if (xwin) {
        xwin->data = this;
        xwin->on_repaint = _on_repaint;
        xwin->on_event = _on_event;
        xwin->on_close = _on_close;
        xwin->on_resize = _on_resize;
        xwin->on_move = _on_move;
        xwin->on_focus = _on_focus;
        xwin->on_unfocus = _on_unfocus;
        return true;
    }
    return false;
}

void XWin::close() {
    if (xwin) {
        xwin_close(xwin);
        xwin = nullptr;
    }
}

void XWin::setVisible(bool visible) {
    if (xwin) {
        xwin_set_visible(xwin, visible);
    }
}

void XWin::repaint() {
    if (xwin) {
        xwin_repaint(xwin);
    }
}

graph_t* XWin::getGraph() {
    if (xwin) {
        return xwin_fetch_graph(xwin, nullptr);
    }
    return nullptr;
}

int XWin::getWidth() const {
    return xwin ? xwin->width : 0;
}

int XWin::getHeight() const {
    return xwin ? xwin->height : 0;
}

// Static callback functions
void XWin::_on_repaint(xwin_t* xwin, graph_t* g) {
    if (xwin && xwin->data) {
        XWin* self = static_cast<XWin*>(xwin->data);
        self->onRepaint(g);
    }
}

void XWin::_on_event(xwin_t* xwin, xevent_t* ev) {
    if (!xwin || !xwin->data || !ev) return;
    
    XWin* self = static_cast<XWin*>(xwin->data);
    
    switch (ev->type) {
        case XEVT_MOUSE:
            self->onMouse(ev);
            break;
        case XEVT_IM:
            self->onIM(ev);
            break;
        default:
            break;
    }
}

bool XWin::_on_close(xwin_t* xwin) {
    if (xwin && xwin->data) {
        XWin* self = static_cast<XWin*>(xwin->data);
        return self->onClose();
    }
    return true;
}

void XWin::_on_resize(xwin_t* xwin) {
    if (xwin && xwin->data) {
        XWin* self = static_cast<XWin*>(xwin->data);
        self->onResize();
    }
}

void XWin::_on_move(xwin_t* xwin) {
    if (xwin && xwin->data) {
        XWin* self = static_cast<XWin*>(xwin->data);
        self->onMove();
    }
}

void XWin::_on_focus(xwin_t* xwin) {
    if (xwin && xwin->data) {
        XWin* self = static_cast<XWin*>(xwin->data);
        self->onFocus();
    }
}

void XWin::_on_unfocus(xwin_t* xwin) {
    if (xwin && xwin->data) {
        XWin* self = static_cast<XWin*>(xwin->data);
        self->onUnfocus();
    }
}

} // namespace Ewok