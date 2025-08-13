# EwokOS XWIN GUI Library - WebAssembly Port

This is a standalone WebAssembly port of the XWIN GUI library from EwokOS, designed to run in web browsers.

## Structure

- `src/` - Source code for the GUI library
  - `core/` - Core windowing system (X library port)
  - `cpp/` - C++ wrapper layer (X++ library port)
  - `widgets/` - High-level widget framework (Widget++ library port)
  - `web/` - WebAssembly/HTML5 integration layer
- `include/` - Header files
- `demo/` - Demo applications
- `build/` - Build output directory

## Dependencies

The library provides web-compatible replacements for:
- EwokOS system calls → Web APIs
- Graphics rendering → HTML5 Canvas
- Font rendering → Web fonts
- Memory management → WebAssembly memory model

## Prerequisites

This project requires Emscripten to compile C/C++ code to WebAssembly.

### Installing Emscripten

1. **Download and install Emscripten SDK:**
   ```bash
   # Clone the emsdk repository
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   
   # Download and install the latest SDK tools
   ./emsdk install latest
   
   # Make the latest SDK "active" for the current user
   ./emsdk activate latest
   
   # Activate PATH and other environment variables in the current terminal
   source ./emsdk_env.sh
   ```

2. **Verify installation:**
   ```bash
   emcc --version
   ```

3. **For permanent setup, add to your shell profile:**
   ```bash
   echo 'source /path/to/emsdk/emsdk_env.sh' >> ~/.bashrc
   # or for zsh users:
   echo 'source /path/to/emsdk/emsdk_env.sh' >> ~/.zshrc
   ```

### Alternative Installation Methods

- **Ubuntu/Debian:** `sudo apt install emscripten`
- **macOS with Homebrew:** `brew install emscripten`
- **Windows:** Download from [emscripten.org](https://emscripten.org/docs/getting_started/downloads.html)

## Building

```bash
make clean
make
```

## Usage

```html
<script src="ewok-gui.js"></script>
<canvas id="canvas" width="800" height="600"></canvas>
<script>
  const gui = new EwokGUI('canvas');
  const window = gui.createWindow(100, 100, 400, 300);
  // ... use GUI components
</script>
```