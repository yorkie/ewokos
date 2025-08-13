# WebAssembly GUI Library Makefile

CC = emcc
CXX = em++
CFLAGS = -I./include -O2 -s WASM=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','setValue','getValue'] \
         -s EXPORTED_FUNCTIONS=['_malloc','_free','_web_handle_mouse_event','_web_handle_key_event'] \
         -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME='EwokGUIModule'

DEMO_CFLAGS = -I./include -O2 -s WASM=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','setValue','getValue'] \
              -s EXPORTED_FUNCTIONS=['_malloc','_free','_web_handle_mouse_event','_web_handle_key_event','_demo_main'] \
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