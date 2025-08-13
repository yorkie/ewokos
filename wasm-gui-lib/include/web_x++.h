#ifndef WEB_X_PLUS_PLUS_H
#define WEB_X_PLUS_PLUS_H

#include "web_x.h"
#include "web_font.h"
#include <string>

namespace Ewok {

class X {
    x_t x;
public:
    inline x_t* c_x(void) { return &x; }
    X(void);
    void run(void (*loop)(void*), void* p = NULL);
    void terminate(void);
    bool terminated(void);
    
    static const char* getResName(const char* name);
    static bool getScreenInfo(int& w, int& h, int index = 0);
};

class XWin {
    xwin_t* xwin;
    
public:
    XWin();
    virtual ~XWin();
    
    bool open(X* x, uint32_t disp_index, int x_pos, int y_pos, int w, int h, const char* title, int style);
    void close();
    void setVisible(bool visible);
    void repaint();
    
    graph_t* getGraph();
    int getWidth() const;
    int getHeight() const;
    
    // Event handlers (virtual functions to override)
    virtual void onRepaint(graph_t* g) { }
    virtual void onMouse(xevent_t* ev) { }
    virtual void onIM(xevent_t* ev) { }
    virtual void onResize() { }
    virtual void onMove() { }
    virtual void onFocus() { }
    virtual void onUnfocus() { }
    virtual bool onClose() { return true; }
    
private:
    static void _on_repaint(xwin_t* xwin, graph_t* g);
    static void _on_event(xwin_t* xwin, xevent_t* ev);
    static bool _on_close(xwin_t* xwin);
    static void _on_resize(xwin_t* xwin);
    static void _on_move(xwin_t* xwin);
    static void _on_focus(xwin_t* xwin);
    static void _on_unfocus(xwin_t* xwin);
};

} // namespace Ewok

#endif // WEB_X_PLUS_PLUS_H