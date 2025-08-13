# WASM Quick Start Guide

## Prerequisites

Install required tools:

```bash
# Check/install GCC
gcc --version

# Check/install Node.js (v12+)
node --version

# If Node.js not installed:
# Ubuntu/Debian:
curl -fsSL https://deb.nodesource.com/setup_lts.x | sudo -E bash -
sudo apt-get install -y nodejs

# macOS:
brew install node

# Windows:
# Download from https://nodejs.org/
```

## Compile WASM Kernel

```bash
# 1. Navigate to WASM kernel directory
cd machine/wasm/kernel

# 2. Clean previous builds
make clean

# 3. Compile kernel
make

# Expected output:
# - kernel.wasm (81KB binary)
# - Compilation warnings are normal
```

## Startup Node.js Example

### Basic Execution

```bash
# Method 1: Host simulation (direct execution)
./kernel.wasm

# Method 2: Node.js runtime
node wasm_runtime.js kernel.wasm
```

### With Optional Components

```bash
# With SD card image
node wasm_runtime.js kernel.wasm root.img

# With SD card and init process
node wasm_runtime.js kernel.wasm root.img init_process
```

### Debug Mode

```bash
# Start with debugger
node --inspect-brk wasm_runtime.js kernel.wasm

# Then open Chrome and go to: chrome://inspect
# Click "Open dedicated DevTools for Node"
```

## Expected Output

Successful startup should show:
```
EwokOS WASM Runtime starting...
EwokOS WASM Host Simulation Starting...
[Kernel initialization messages...]
```

## Troubleshooting

### Compilation Issues
- **"gcc: command not found"** → Install build tools
- **Missing headers** → Check include paths in make.rule

### Runtime Issues  
- **"node: command not found"** → Install Node.js
- **WebAssembly compile error** → Currently using host simulation (normal)
- **Segmentation fault** → Use `gdb ./kernel.wasm` to debug

## Next Steps

- Read `machine/wasm/kernel/README.md` for detailed documentation
- See `docs/wasm/README.md` for comprehensive guide
- Try modifying kernel source in `kernel/platform/wasm/`