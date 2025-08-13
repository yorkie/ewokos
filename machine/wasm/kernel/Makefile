include config.mk

# WASM output files
OS_IMG = kernel.wasm
OS_ELF = kernel.wasm

EWOKOS_ROOT_DIR = ../../..
MACHINE = wasm

# Use WASM platform build rules
include $(EWOKOS_ROOT_DIR)/kernel/platform/$(ARCH)/make.rule

# Build targets
asm:
	@echo "WASM disassembly not supported"

run:
	node wasm_runtime.js $(OS_IMG)

debug:
	node --inspect-brk wasm_runtime.js $(OS_IMG)

clean_extra:
	rm -f *.wasm *.js *.html