#include "x++/X.h"
#include "x++/XWin.h"
#include "graph/graph.h"
#include "font/font.h"
#include <cstdio>

using namespace Ewok;

class DemoWin : public XWin {
private:
    font_t* font;
    
public:
    DemoWin() : font(nullptr) {
        font = font_new("Arial", false);
    }
    
    ~DemoWin() {
        if (font) {
            font_free(font);
        }
    }
    
    void onRepaint(graph_t* g) override {
        if (!g) return;
        
        // Clear with light blue background
        graph_clear(g, 0xffe6f3ff);
        
        // Draw title bar
        graph_fill(g, 0, 0, getW(), 30, 0xff4a90e2);
        
        // Draw some simple text using the basic text implementation
        graph_draw_text(g, 10, 20, "EwokOS GUI Demo - C++", font, 16, 0xffffffff);
        graph_draw_text(g, 10, 60, "Migrated from original xwin source!", font, 14, 0xff000000);
        
        // Draw some colored rectangles
        graph_fill(g, 50, 100, 100, 50, 0xffff0000);  // Red
        graph_fill(g, 170, 100, 100, 50, 0xff00ff00); // Green  
        graph_fill(g, 290, 100, 100, 50, 0xff0000ff); // Blue
        
        // Draw circles
        graph_fill_circle(g, 100, 200, 30, 0xffffff00); // Yellow circle
        graph_fill_circle(g, 220, 200, 30, 0xffff00ff); // Magenta circle
        graph_fill_circle(g, 340, 200, 30, 0xff00ffff); // Cyan circle
    }
    
    bool onClose() override {
        return true; // Allow close
    }
};

// Main demo function
extern "C" int demo_main() {
    X x;
    
    DemoWin* win = new DemoWin();
    if (!win->open(&x, 0, 100, 100, 500, 300, "EwokOS WebAssembly Demo", XWIN_STYLE_NORMAL)) {
        delete win;
        return -1;
    }
    
    win->setVisible(true);
    win->repaint();
    
    // In WebAssembly, we don't run a traditional event loop
    // The browser will handle repainting as needed
    
    return 0;
}