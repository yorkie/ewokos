# EwokOS WASM Platform

WebAssembly (WASM) platform support for EwokOS, enabling the kernel to run in WASM-capable environments including web browsers and Node.js runtime.

## Current Implementation Status

The WASM platform currently supports **host simulation mode** using standard GCC compilation, which allows for rapid development and testing of the WASM platform architecture. Future versions will support true WASM compilation using WASM toolchains.

## Build Requirements

### Host Simulation Mode (Current)
- **GCC** - Standard GNU Compiler Collection
- **Node.js** - Version 12 or higher for runtime support
- **Standard Unix tools** - make, ar, objcopy, objdump

### Future WASM Mode
- **WASI SDK** or **Emscripten** for true WASM compilation
- **wasmtime** or **wasmer** for WASM runtime (alternative to Node.js)

## Building the WASM Kernel

### 1. Host Simulation Build

```bash
cd machine/wasm/kernel
make clean
make
```

This creates `kernel.wasm` which is actually an ELF executable that simulates WASM behavior.

### 2. Build Output

The build process generates:
- `kernel.wasm` - 81KB kernel binary (host simulation)
- `wasm_runtime.js` - Node.js runtime environment

## Running the WASM Kernel

### Host Simulation Mode

```bash
# Direct execution (host simulation)
cd machine/wasm/kernel
./kernel.wasm

# Node.js runtime simulation
node wasm_runtime.js kernel.wasm
```

### With Optional Components

```bash
# With SD card image and init process
node wasm_runtime.js kernel.wasm [sd_image.img] [init_process]
```

## Node.js Runtime Features

The `wasm_runtime.js` provides a complete runtime environment:

### Core Services
- **UART Simulation** - Console I/O with interactive input
- **Timer Simulation** - 10ms timer interrupts
- **Memory Management** - WASM memory interface
- **Debug Output** - Console logging and debugging

### Storage Support
- **SD Card Simulation** - File system operations
- **Block-level I/O** - 512-byte block operations
- **File system mounting** - EXT2 image support

### Development Features
- **Debug Mode** - Run with `node --inspect-brk wasm_runtime.js kernel.wasm`
- **Interactive Console** - Real-time UART input/output
- **Hot Reloading** - Restart runtime without rebuilding

## Runtime Options

### Debug Mode
```bash
node --inspect-brk wasm_runtime.js kernel.wasm
# Then open Chrome DevTools at chrome://inspect
```

### With SD Card Image
```bash
# Prepare SD card image (example)
dd if=/dev/zero of=root.ext2 bs=1024 count=16384
mke2fs -b 1024 -I 128 root.ext2

# Run with SD image
node wasm_runtime.js kernel.wasm root.ext2
```

## Platform Architecture

### Memory Model
- **Flat Memory Space** - No MMU complexity
- **Identity Mapping** - Virtual = Physical addresses
- **Host Allocation** - Memory managed by host environment

### Interrupt Simulation
- **Timer Interrupts** - Host-provided timing
- **UART Interrupts** - Input-driven interrupts
- **Software Interrupts** - System call interface

### I/O Abstraction
- **UART** - Console input/output
- **Timer** - Host system time
- **SD Card** - File-based block device
- **IRQ Controller** - Simulated interrupt handling

## Development Workflow

### 1. Modify Kernel Code
```bash
# Edit kernel source files
vim ../../../kernel/platform/wasm/arch/wasm32/system.c
```

### 2. Rebuild and Test
```bash
make clean && make
./kernel.wasm  # Quick host test
```

### 3. Test with Runtime
```bash
node wasm_runtime.js kernel.wasm
```

### 4. Debug Issues
```bash
node --inspect-brk wasm_runtime.js kernel.wasm
# Use Chrome DevTools for debugging
```

## Troubleshooting

### Build Failures
- Ensure GCC is installed and accessible
- Check include paths in make.rule
- Verify all dependencies are available

### Runtime Issues
- Check Node.js version (requires v12+)
- Verify kernel.wasm file exists and is readable
- Use debug mode for detailed error information

### Performance Issues
- Host simulation has overhead compared to native execution
- Use timer adjustments in wasm_runtime.js for different performance profiles

## Future Development

### True WASM Compilation
- Migrate from GCC to WASI SDK or Emscripten
- Generate actual .wasm bytecode
- Support for WebAssembly System Interface (WASI)

### Web Browser Support
- Add HTML wrapper for browser execution
- WebGL integration for framebuffer support
- WebAudio for sound device simulation

### Enhanced Runtime
- Multiple WASM instance support
- Hot code reloading
- Performance profiling tools

## File Structure

```
machine/wasm/kernel/
├── README.md           # This documentation
├── Makefile           # Build configuration
├── config.mk          # Platform configuration
├── make.bsp           # Board support package config
├── wasm_runtime.js    # Node.js runtime environment
├── kernel.wasm        # Compiled kernel (generated)
└── bsp/               # Board support package
    └── (BSP files)
```

## See Also

- Main EwokOS documentation: `../../../README.md`
- WASM platform source: `../../../kernel/platform/wasm/`
- General build documentation: `../../../docs/`