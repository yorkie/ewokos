# EwokOS WASM Platform - Detailed Documentation

This directory contains comprehensive documentation for the EwokOS WebAssembly (WASM) platform implementation.

## Quick Start

### Prerequisites
```bash
# Verify required tools
gcc --version          # GNU Compiler Collection
node --version         # Node.js (v12+)
make --version         # GNU Make
```

### Build and Run
```bash
# Clone repository and navigate to WASM kernel
git clone https://github.com/yorkie/ewokos.git
cd ewokos/machine/wasm/kernel

# Build the kernel
make clean
make

# Run in host simulation mode
./kernel.wasm

# Or run with Node.js runtime
node wasm_runtime.js kernel.wasm
```

## Current Implementation

### Architecture Overview

The WASM platform provides a simplified OS environment suitable for:
- Educational purposes
- Cross-platform development
- Web-based demonstrations
- Rapid prototyping

### Key Features

1. **Host Simulation Mode**
   - Uses standard GCC compilation
   - Enables rapid development and debugging
   - Maintains WASM semantics without WASM overhead

2. **Flat Memory Model**
   - No MMU complexity
   - Identity mapping (virtual = physical)
   - Simplified memory management

3. **Simulated Hardware**
   - UART through console I/O
   - Timer using host system time
   - SD card via file system operations
   - IRQ through host callbacks

### Build System Integration

```
machine/wasm/kernel/
├── Makefile           → Main build file
├── config.mk         → Platform configuration 
└── make.bsp          → Board support package

kernel/platform/wasm/
├── make.rule         → Build rules and compiler flags
└── arch/
    ├── wasm32/       → 32-bit WASM architecture
    └── common/       → Shared platform code
```

## Compilation Process

### Current: Host Simulation

```bash
# Compilation flags (from make.rule)
CC = gcc
CFLAGS = -DWASM_PLATFORM -DHOST_SIMULATION -O2 ...

# Linking creates ELF executable (not WASM bytecode)
gcc $(OBJS) -o kernel.wasm
```

### Future: True WASM Compilation

```bash
# With WASI SDK
export WASI_SDK_PATH=/path/to/wasi-sdk
CC = $WASI_SDK_PATH/bin/clang --sysroot=$WASI_SDK_PATH/share/wasi-sysroot
CFLAGS = --target=wasm32-wasi -O2 ...

# With Emscripten
CC = emcc
CFLAGS = -s WASM=1 -s EXPORTED_FUNCTIONS='["_main"]' ...
```

## Node.js Runtime Environment

### Overview

The `wasm_runtime.js` provides a complete execution environment for the EwokOS kernel, simulating hardware devices and providing system services.

### Core Components

1. **EwokOSWASMRuntime Class**
   - Main runtime controller
   - Manages WASM instance lifecycle
   - Provides import functions for kernel

2. **Import Functions** (callable from kernel)
   ```javascript
   wasm_debug_print(strPtr, len)    // Console output
   wasm_get_time_ms()               // System time
   wasm_exit(code)                  // Clean shutdown
   wasm_uart_has_data()             // UART input check
   wasm_uart_read_char()            // UART input read
   wasm_sd_read_block(...)          // SD card read
   wasm_sd_write_block(...)         // SD card write
   ```

3. **Host Services**
   - UART I/O through process.stdin/stdout
   - Timer interrupts via setInterval
   - SD card simulation through Buffer operations
   - Memory management helpers

### Usage Examples

#### Basic Execution
```bash
node wasm_runtime.js kernel.wasm
```

#### With SD Card Image
```bash
# Create test SD image
dd if=/dev/zero of=test.img bs=1M count=16
mke2fs -b 1024 test.img

# Run with SD support
node wasm_runtime.js kernel.wasm test.img
```

#### Debug Mode
```bash
# Start with debugger
node --inspect-brk wasm_runtime.js kernel.wasm

# Connect Chrome DevTools:
# 1. Open Chrome
# 2. Go to chrome://inspect
# 3. Click "Open dedicated DevTools for Node"
```

#### With Init Process
```bash
# Compile init process
cd ../../../system/sbin/init
make

# Run with init
cd ../../../machine/wasm/kernel
node wasm_runtime.js kernel.wasm root.img ../../../system/sbin/init/init
```

## Development Workflow

### 1. Kernel Development

```bash
# Edit platform-specific code
vim ../../../kernel/platform/wasm/arch/wasm32/system.c

# Edit kernel core (if needed)
vim ../../../kernel/kernel/src/kernel.c

# Rebuild
make clean && make
```

### 2. Runtime Development

```bash
# Edit runtime
vim wasm_runtime.js

# Test changes
node wasm_runtime.js kernel.wasm
```

### 3. Testing and Debugging

```bash
# Quick functionality test
./kernel.wasm

# Full runtime test
node wasm_runtime.js kernel.wasm

# Debug kernel issues
node --inspect-brk wasm_runtime.js kernel.wasm

# Debug with GDB (host simulation)
gdb ./kernel.wasm
```

## Platform-Specific Implementation

### Memory Management

```c
// kernel/platform/wasm/arch/wasm32/mmu.c

// Simplified heap allocation for WASM
static char wasm_heap[WASM_HEAP_SIZE];
static uint32_t heap_pos = 0;

uint32_t wasm_kalloc(uint32_t size) {
    if (heap_pos + size > WASM_HEAP_SIZE) {
        return 0; // Out of memory
    }
    uint32_t addr = (uint32_t)&wasm_heap[heap_pos];
    heap_pos += ALIGN_UP(size, 4);
    return addr;
}
```

### UART Implementation

```c
// kernel/platform/wasm/arch/wasm32/system.c

// Host simulation UART
void uart_putc(int c) {
#ifdef HOST_SIMULATION
    char ch = c;
    write(1, &ch, 1);  // Direct console output
#else
    wasm_debug_print(&ch, 1);  // WASM import function
#endif
}
```

### Timer Implementation

```c
uint64_t system_time() {
#ifdef HOST_SIMULATION
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#else
    return wasm_get_time_ms();  // WASM import function
#endif
}
```

## Future Roadmap

### Phase 1: True WASM Compilation
- [ ] Integrate WASI SDK build option
- [ ] Generate actual .wasm bytecode
- [ ] Test with wasmtime/wasmer runtimes
- [ ] Benchmark performance vs. host simulation

### Phase 2: Web Browser Support
- [ ] Create HTML wrapper for browser execution
- [ ] Add WebGL framebuffer support
- [ ] Implement WebAudio for sound devices
- [ ] Add file upload/download for SD card operations

### Phase 3: Enhanced Features
- [ ] Multi-threading support (Web Workers)
- [ ] Network I/O simulation
- [ ] Performance profiling tools
- [ ] Hot code reloading for development

### Phase 4: Educational Tools
- [ ] Interactive kernel debugger
- [ ] Memory visualization
- [ ] System call tracing
- [ ] Process execution visualization

## Integration with Existing EwokOS

### Kernel Compatibility

The WASM platform maintains compatibility with the EwokOS kernel API:

```c
// Standard EwokOS calls work unchanged
proc_t* proc = proc_create();
int pid = proc_start(proc, "/sbin/init", NULL);
```

### System Services

Many EwokOS system services can run on WASM with minimal adaptation:

- Virtual File System (VFS)
- Process management
- IPC mechanisms
- Device drivers (with simulation layer)

### Root File System

The WASM platform can use standard EwokOS root file systems:

```bash
# Build standard rootfs
cd system
make basic

# Use with WASM
cd ../machine/wasm/kernel
node wasm_runtime.js kernel.wasm ../../../system/root.ext2
```

## Troubleshooting Guide

### Build Issues

**Problem**: `make: gcc: command not found`
```bash
# Solution: Install GCC
sudo apt-get install build-essential  # Ubuntu/Debian
brew install gcc                      # macOS
```

**Problem**: Missing header files
```bash
# Solution: Check include paths
cd machine/wasm/kernel
grep -r "include.*\.h" ../../../kernel/platform/wasm/
```

### Runtime Issues

**Problem**: `node: command not found`
```bash
# Solution: Install Node.js
curl -fsSL https://deb.nodesource.com/setup_lts.x | sudo -E bash -
sudo apt-get install -y nodejs
```

**Problem**: `Error: Cannot read property 'exports' of undefined`
```bash
# Solution: Check WASM module compilation
file kernel.wasm  # Should show ELF for host simulation
```

**Problem**: Segmentation fault in host simulation
```bash
# Solution: Debug with GDB
gdb ./kernel.wasm
(gdb) run
(gdb) bt  # Get stack trace when it crashes
```

### Performance Issues

**Problem**: Slow execution
- Host simulation has overhead
- Adjust timer intervals in wasm_runtime.js
- Use `-O2` optimization in build

**Problem**: High memory usage
- Check for memory leaks in kernel code
- Monitor heap usage in wasm_runtime.js
- Use smaller heap sizes for testing

## Contributing

### Code Style
- Follow existing EwokOS coding conventions
- Use descriptive variable names
- Add comments for WASM-specific code
- Test both host simulation and (future) true WASM modes

### Testing
- Test build process on multiple platforms
- Verify runtime functionality
- Check compatibility with existing EwokOS components
- Document any platform-specific limitations

### Documentation
- Update this documentation for new features
- Add examples for new functionality
- Include troubleshooting for common issues
- Maintain compatibility information

## References

- [WebAssembly Specification](https://webassembly.github.io/spec/)
- [WASI (WebAssembly System Interface)](https://wasi.dev/)
- [Node.js WebAssembly Documentation](https://nodejs.org/api/wasm.html)
- [Emscripten Documentation](https://emscripten.org/docs/)
- [WASI SDK](https://github.com/WebAssembly/wasi-sdk)