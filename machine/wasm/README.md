# EwokOS WebAssembly Implementation

This directory contains the WebAssembly (WASM) implementation of EwokOS, allowing the microkernel operating system to run in web browsers using Emscripten compilation.

## Overview

EwokOS has been successfully adapted to compile to WebAssembly, providing an interactive browser-based demonstration of the microkernel operating system concepts. This implementation showcases:

- Process management and scheduling
- Inter-process communication concepts
- Virtual filesystem principles
- Device service architecture
- Microkernel design patterns

## Directory Structure

```
machine/wasm/
├── demo/           # Working WASM demo (recommended starting point)
├── kernel/         # Full kernel WASM platform (work in progress)
└── README.md       # This file
```

## Quick Start - Demo

The easiest way to experience EwokOS in the browser is through the demo:

```bash
# Navigate to demo directory
cd machine/wasm/demo

# Build the demo
make

# Start local web server
make serve

# Open browser to http://localhost:8080
```

### Demo Features

- **Interactive Terminal**: Command-line interface with EwokOS commands
- **Process Management**: Create and list processes (`ps`, `create`)
- **System Information**: View system configuration (`info`)
- **Scheduler Simulation**: Trigger scheduler ticks (`tick`)
- **Help System**: Built-in command help (`help`)

### Available Commands

- `ps` - List running processes
- `help` - Show available commands
- `info` - Display system information
- `create` - Create a new demo process
- `tick` - Trigger scheduler tick
- `clear` - Clear console output

## Architecture

### WASM Platform Layer

The WASM implementation provides platform-specific abstractions:

- **Memory Management**: Uses browser's memory model instead of ARM MMU
- **Process Simulation**: Simplified process structures for demonstration
- **Hardware Abstraction**: Browser APIs replace physical hardware
- **JavaScript Integration**: Seamless C-JavaScript interoperability

### Build System

Uses Emscripten for compilation:
- C code compiled to WebAssembly
- JavaScript glue code for browser integration
- HTML interface for user interaction
- Exported functions for browser-kernel communication

## Development

### Prerequisites

- Emscripten SDK installed (`emcc` available)
- Python 3 (for local web server)
- Modern web browser with WebAssembly support

### Building

```bash
# Clean build
make clean

# Build WASM demo
make

# Serve locally
make serve
```

### Customization

The demo can be extended by:
1. Modifying `wasm_demo.c` for new functionality
2. Updating `wasm_demo.js` for browser integration
3. Enhancing `ewokos_demo.html` for UI improvements

## Technical Details

### Compilation Flags

- `-s WASM=1`: Enable WebAssembly output
- `-s EXPORTED_FUNCTIONS`: Export C functions to JavaScript
- `-s EXPORTED_RUNTIME_METHODS`: Export Emscripten runtime functions
- `--pre-js`: Include JavaScript code before WASM loading

### Memory Model

- Initial memory: 16MB
- Memory growth allowed for dynamic allocation
- Stack size: 1MB
- Heap management through Emscripten

### Browser Compatibility

Requires browsers with:
- WebAssembly support (all modern browsers)
- JavaScript ES6+ features
- Canvas/HTML5 support for UI

## Comparison with Native EwokOS

| Feature | Native ARM | WASM Browser |
|---------|------------|--------------|
| Process Management | Full implementation | Simplified demo |
| Memory Management | ARM MMU | Browser heap |
| Hardware Access | Direct ARM registers | Browser APIs |
| File System | ext2 on SD card | Simulated |
| Device Drivers | Real hardware | Emulated |
| Performance | Native speed | Near-native via WASM |

## Future Enhancements

Potential improvements:
- File system persistence using browser storage
- Network communication via WebRTC
- Audio support through Web Audio API
- Graphics rendering using Canvas/WebGL
- Multi-threading via Web Workers

## Author

Original EwokOS: Misa.Z (misa.zhu@gmail.com)
WASM Implementation: Demonstrated microkernel concepts in browser environment

## License

Same as original EwokOS project.