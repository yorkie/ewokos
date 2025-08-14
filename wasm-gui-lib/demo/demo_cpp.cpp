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
            snprintf(keyText, sizeof(keyText), "Last key: %d (0x%x)", keyPressed, keyPressed);
            graph_draw_text(g, 10, 140, keyText, font, 12, 0xff666666);
        }
        
        // Draw a simple button
        graph_fill(g, 50, 180, 100, 30, 0xfff0f0f0);
        graph_box(g, 50, 180, 100, 30, 0xff888888);
        graph_draw_text(g, 80, 200, "Button", font, 14, 0xff000000);
        
        // Draw some shapes
        graph_fill_circle(g, 200, 200, 20, 0xffff4444);
        graph_circle(g, 250, 200, 25, 0xff4444ff);
        graph_line(g, 50, 250, 200, 280, 0xff44ff44);
        
        // Draw a pattern
        for (int i = 0; i < 10; i++) {
            uint32_t color = 0xff000000 | (i * 25 << 16) | (i * 25 << 8) | (255 - i * 25);
            graph_fill(g, 300 + i * 15, 150 + i * 5, 10, 50, color);
        }
    }
    
    void onMouse(xevent_t* ev) override {
        if (!ev) return;
        
        mouseX = ev->value.mouse.x;
        mouseY = ev->value.mouse.y;
        mouseDown = (ev->state == 1); // mouse down
        
        // Check if clicked on button
        if (ev->state == 1 && mouseX >= 50 && mouseX <= 150 && mouseY >= 180 && mouseY <= 210) {
            // Button clicked - change background or do something
            printf("Button clicked at (%d, %d)\n", mouseX, mouseY);
        }
        
        repaint();
    }
    
    void onIM(xevent_t* ev) override {
        if (!ev) return;
        
        keyPressed = ev->value.im.value;
        printf("Key pressed: %d\n", keyPressed);
        
        repaint();
    }
    
    void onResize() override {
        printf("Window resized to %dx%d\n", getWidth(), getHeight());
        repaint();
    }
    
    bool onClose() override {
        printf("Window close requested\n");
        return true; // Allow close
    }
};

// Main function that will be called from JavaScript
extern "C" {
    void demo_main() {
        // Initialize the system
        X x;
        DemoWin win;
        
        // Setup canvas
        web_x_init_canvas("canvas");
        web_x_setup_event_handlers("canvas");
        
        // Open window
        if (win.open(&x, 0, 0, 0, 600, 400, "Demo Window", 0)) {
            win.setVisible(true);
            
            // Start the main loop
            x.run(nullptr, nullptr);
        }
    }
}

int main() {
    demo_main();
    return 0;
}