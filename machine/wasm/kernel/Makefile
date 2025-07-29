include config.mk

OS_IMG = kernel.wasm
OS_ELF = kernel.js

EWOKOS_ROOT_DIR = ../../..

include make.bsp
include $(EWOKOS_ROOT_DIR)/kernel/platform/wasm/make.rule

# WASM-specific targets
run: 
	@echo "Please open browser and navigate to index.html to run ewokos"

serve:
	python3 -m http.server 8080

clean_wasm:
	rm -f $(BUILD_DIR)/$(OS_IMG) $(BUILD_DIR)/$(OS_ELF) $(BUILD_DIR)/index.html