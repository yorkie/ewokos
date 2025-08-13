# WASM platform configuration
ARCH = wasm
ARCH_VER = wasm32
MACHINE = wasm

# WASM doesn't use physical loading address
LOAD_ADDRESS = 0x0

# No SMP support for initial WASM implementation
SMP = no

# WASM platform specific settings
WASM_STACK_SIZE = 1048576  # 1MB stack