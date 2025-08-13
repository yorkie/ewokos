# WebAssembly GUI Library Makefile

CC = emcc
CXX = em++
CFLAGS = -I./include -O2 -s WASM=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','setValue','getValue'] \
         -s EXPORTED_FUNCTIONS=['_malloc','_free','_web_handle_mouse_event','_web_handle_key_event','_web_x_init_canvas','_web_x_setup_event_handlers','_web_graph_flush_to_canvas','_x_init','_x_run','_x_terminate','_x_push_event','_xwin_open','_xwin_close','_xwin_destroy','_xwin_fetch_graph','_xwin_set_visible','_xwin_repaint','_xwin_move_to','_xwin_resize_to','_graph_new','_graph_free','_graph_init','_graph_clear','_graph_fill','_graph_pixel','_graph_pixel_safe','_graph_line','_graph_box','_graph_circle','_graph_fill_circle','_graph_blt','_graph_blt_alpha','_graph_draw_text','_graph_draw_text_font','_graph_get_pixel','_graph_set_clip','_graph_unset_clip','_font_new','_font_free','_font_init','_font_load','_font_text_size'] \
         -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME='EwokGUIModule'

DEMO_CFLAGS = -I./include -O2 -s WASM=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','setValue','getValue'] \
              -s EXPORTED_FUNCTIONS=['_malloc','_free','_web_handle_mouse_event','_web_handle_key_event','_demo_main','_web_x_init_canvas','_web_x_setup_event_handlers','_web_graph_flush_to_canvas','_x_init','_x_run','_x_terminate','_x_push_event','_xwin_open','_xwin_close','_xwin_destroy','_xwin_fetch_graph','_xwin_set_visible','_xwin_repaint','_xwin_move_to','_xwin_resize_to','_graph_new','_graph_free','_graph_init','_graph_clear','_graph_fill','_graph_pixel','_graph_pixel_safe','_graph_line','_graph_box','_graph_circle','_graph_fill_circle','_graph_blt','_graph_blt_alpha','_graph_draw_text','_graph_draw_text_font','_graph_get_pixel','_graph_set_clip','_graph_unset_clip','_font_new','_font_free','_font_init','_font_load','_font_text_size'] \
              -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME='EwokGUIModule'

CXXFLAGS = $(DEMO_CFLAGS) -std=c++14

SRCDIR = src
BUILDDIR = build
INCLUDEDIR = include

# Core sources
CORE_SRCS = $(SRCDIR)/core/wasm_types.c \
            $(SRCDIR)/core/web_graph.c \
            $(SRCDIR)/core/web_x.c \
            $(SRCDIR)/core/web_font.c

# C++ sources  
CPP_SRCS = $(SRCDIR)/cpp/web_x++.cpp

# Demo sources
DEMO_SRCS = demo/demo_cpp.cpp

# Build targets
LIB_JS = $(BUILDDIR)/ewok-gui.js
LIB_WASM = $(BUILDDIR)/ewok-gui.wasm
DEMO_JS = $(BUILDDIR)/demo-cpp.js
DEMO_WASM = $(BUILDDIR)/demo-cpp.wasm

.PHONY: all clean demo cpp-demo

all: $(BUILDDIR) $(LIB_JS)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(LIB_JS): $(CORE_SRCS)
	$(CC) $(CFLAGS) -o $@ $(CORE_SRCS)

$(DEMO_JS): $(CORE_SRCS) $(CPP_SRCS) $(DEMO_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $(CORE_SRCS) $(CPP_SRCS) $(DEMO_SRCS)

demo: $(LIB_JS)
	cp $(LIB_JS) demo/
	cp $(LIB_WASM) demo/

cpp-demo: $(DEMO_JS)
	cp $(DEMO_JS) demo/
	cp $(DEMO_WASM) demo/

clean:
	rm -rf $(BUILDDIR)
	rm -f demo/ewok-gui.js demo/ewok-gui.wasm
	rm -f demo/demo-cpp.js demo/demo-cpp.wasm

install: $(LIB_JS)
	cp $(LIB_JS) /usr/local/lib/
	cp $(LIB_WASM) /usr/local/lib/
	cp -r $(INCLUDEDIR)/* /usr/local/include/

.SUFFIXES: .c .o .cpp .o
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@