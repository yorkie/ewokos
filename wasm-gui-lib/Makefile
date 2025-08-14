# WebAssembly GUI Library Makefile - Using Original EwokOS Sources

CC = emcc
CXX = em++

# Include paths for original EwokOS sources and WebAssembly compatibility layer
INCLUDES = -I./include \
           -I../system/gui/libs/graph/include \
           -I../system/gui/libs/font/include \
           -I../system/xwin/libs/x/include \
           -I../system/xwin/libs/x++/include

CFLAGS = $(INCLUDES) -O2 -s WASM=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','setValue','getValue'] \
         -s EXPORTED_FUNCTIONS=['_malloc','_free','_web_handle_mouse_event','_web_handle_key_event','_web_x_init_canvas','_web_x_setup_event_handlers','_web_graph_flush_to_canvas','_x_init','_x_run','_x_terminate','_x_push_event','_xwin_open','_xwin_close','_xwin_destroy','_xwin_fetch_graph','_xwin_set_visible','_xwin_repaint','_xwin_move_to','_xwin_resize_to','_graph_new','_graph_free','_graph_init','_graph_clear','_graph_fill','_graph_pixel','_graph_pixel_safe','_graph_line','_graph_box','_graph_circle','_graph_fill_circle','_graph_blt','_graph_blt_alpha','_graph_draw_text','_graph_draw_text_font','_graph_get_pixel','_graph_set_clip','_graph_unset_clip','_font_new','_font_free','_font_init','_font_load','_font_text_size'] \
         -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME='EwokGUIModule'

DEMO_CFLAGS = $(INCLUDES) -O2 -s WASM=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','setValue','getValue'] \
              -s EXPORTED_FUNCTIONS=['_malloc','_free','_demo_main','_x_init','_x_run','_x_terminate','_x_push_event','_xwin_open','_xwin_close','_xwin_destroy','_xwin_fetch_graph','_xwin_set_visible','_xwin_repaint','_xwin_move_to','_xwin_resize_to','_graph_new','_graph_free','_graph_init','_graph_clear','_graph_fill','_graph_pixel','_graph_pixel_safe','_graph_line','_graph_box','_graph_circle','_graph_fill_circle','_graph_blt','_graph_blt_alpha','_graph_draw_text','_graph_draw_text_font','_graph_get_pixel','_graph_set_clip','_graph_unset_clip','_font_new','_font_free','_font_init','_font_load','_font_text_size'] \
              -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME='EwokGUIModule'

CXXFLAGS = $(DEMO_CFLAGS) -std=c++14

SRCDIR = src
BUILDDIR = build
INCLUDEDIR = include

# WebAssembly compatibility layer
COMPAT_SRCS = $(SRCDIR)/ewoksys_wasm_compat.c

# Original EwokOS X window sources (compiled directly from system/xwin)
XWIN_SRCS = ../system/xwin/libs/x/src/x.c \
            ../system/xwin/libs/x/src/xwin.c

# Original EwokOS graphics library sources (compiled directly from system/gui)
GRAPH_SRCS = ../system/gui/libs/graph/src/graph.c \
             ../system/gui/libs/graph/src/graph_ex.c \
             ../system/gui/libs/graph/src/pixel.c \
             ../system/gui/libs/graph/src/line.c \
             ../system/gui/libs/graph/src/circle.c \
             ../system/gui/libs/graph/src/box.c \
             ../system/gui/libs/graph/src/blt.c \
             ../system/gui/libs/graph/src/arc.c \
             ../system/gui/libs/graph/src/round.c \
             ../system/gui/libs/graph/src/bitmap.c

# Original EwokOS font library sources (compiled directly from system/gui)
# Note: Skipping font.c for now due to FreeType dependencies
# FONT_SRCS = ../system/gui/libs/font/src/font.c
FONT_SRCS =

# Core sources (original EwokOS + WebAssembly compatibility layer)
CORE_SRCS = $(COMPAT_SRCS) $(XWIN_SRCS) $(GRAPH_SRCS) $(FONT_SRCS)

# Original EwokOS C++ sources (compiled directly from system/xwin)
CPP_SRCS = ../system/xwin/libs/x++/src/X.cc \
           ../system/xwin/libs/x++/src/XWin.cc \
           ../system/xwin/libs/x++/src/XTheme.cc

# Demo sources
DEMO_SRCS = demo/demo_cpp_simple.cpp

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