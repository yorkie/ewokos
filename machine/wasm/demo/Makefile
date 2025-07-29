# EwokOS WASM Demo

# Simplified ewokos demo for WASM
CC = emcc
CFLAGS = -O2 -s WASM=1 \
	-s EXPORTED_FUNCTIONS='["_main", "_malloc", "_free", "_get_console_output", "_clear_console", "_demo_command"]' \
	-s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "UTF8ToString", "stringToUTF8", "lengthBytesUTF8"]' \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s INITIAL_MEMORY=16777216 \
	-s NO_EXIT_RUNTIME=1 \
	--pre-js wasm_demo.js

SOURCES = wasm_demo.c
OUTPUT = ewokos_demo.js

all: $(OUTPUT) index.html

$(OUTPUT): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(OUTPUT)

index.html: ewokos_demo.html
	cp ewokos_demo.html index.html

serve:
	python3 -m http.server 8080

clean:
	rm -f $(OUTPUT) ewokos_demo.wasm index.html

.PHONY: all serve clean